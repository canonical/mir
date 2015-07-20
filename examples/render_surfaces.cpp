/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "server_example_input_event_filter.h"
#include "server_example_display_configuration_policy.h"
#include "mir/server_status_listener.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangles.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/options/option.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/scene/surface_factory.h"
#include "mir/frontend/buffer_sink.h"
#include "mir/server.h"
#include "mir/report_exception.h"

#include "mir_image.h"
#include "buffer_render_target.h"
#include "image_renderer.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <sstream>
#include <vector>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mo = mir::options;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::tools;
namespace me = mir::examples;


///\page render_surfaces-example render_surfaces.cpp: A simple program using the mir library.
///\tableofcontents
///render_surfaces shows the use of mir to render some moving surfaces
///\section main main()
/// The main() function uses a RenderSurfacesServerConfiguration to initialize and run mir.
/// \snippet render_surfaces.cpp main_tag
///\section RenderSurfacesServerConfiguration RenderSurfacesServerConfiguration
/// The configuration stubs out client connectivity and input.
/// \snippet render_surfaces.cpp RenderSurfacesServerConfiguration_stubs_tag
/// it also provides a bespoke display buffer compositor
/// \snippet render_surfaces.cpp RenderSurfacesDisplayBufferCompositor_tag
///\section Utilities Utility classes
/// For smooth animation we need to track time and move surfaces accordingly
///\subsection StopWatch StopWatch
/// \snippet render_surfaces.cpp StopWatch_tag
///\subsection Moveable Moveable
/// \snippet render_surfaces.cpp Moveable_tag

///\example render_surfaces.cpp A simple program using the mir library.

namespace
{
std::atomic<bool> created{false};

static const float min_alpha = 0.3f;

char const* const surfaces_to_render = "surfaces-to-render";

///\internal [StopWatch_tag]
// tracks elapsed time - for animation.
class StopWatch
{
public:
    StopWatch() : start(std::chrono::high_resolution_clock::now()),
                  last(start),
                  now(last)
    {
    }

    void stop()
    {
        now = std::chrono::high_resolution_clock::now();
    }

    double elapsed_seconds_since_start()
    {
        auto elapsed = now - start;
        float elapsed_sec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000000.0f;
        return elapsed_sec;
    }

    double elapsed_seconds_since_last_restart()
    {
        auto elapsed = now - last;
        float elapsed_sec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000000.0f;
        return elapsed_sec;
    }

    void restart()
    {
        std::swap(last, now);
    }

private:
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point last;
    std::chrono::high_resolution_clock::time_point now;
};
///\internal [StopWatch_tag]

///\internal [Moveable_tag]
// Adapter to support movement of surfaces.
class Moveable
{
public:
    Moveable() {}
    Moveable(std::shared_ptr<ms::Surface> const& s, const geom::Size& display_size,
             float dx, float dy, const glm::vec3& rotation_axis, float alpha_offset)
        : surface(s), display_size(display_size),
          x{s->top_left().x.as_float()},
          y{s->top_left().y.as_float()},
          w{s->size().width.as_float()},
          h{s->size().height.as_float()},
          dx{dx},
          dy{dy},
          rotation_axis(rotation_axis),
          alpha_offset{alpha_offset}
    {
    }

    void step()
    {
        stop_watch.stop();
        float elapsed_sec = stop_watch.elapsed_seconds_since_last_restart();
        float total_elapsed_sec = stop_watch.elapsed_seconds_since_start();
        stop_watch.restart();

        bool should_update = true;
        float new_x = x + elapsed_sec * dx;
        float new_y = y + elapsed_sec * dy;
        if (new_x < 0.0 || new_x + w > display_size.width.as_uint32_t())
        {
            dx = -dx;
            should_update = false;
        }

        if (new_y < 0.0 || new_y + h > display_size.height.as_uint32_t())
        {
            dy = -dy;
            should_update = false;
        }

        if (should_update)
        {
            surface->move_to({new_x, new_y});
            x = new_x;
            y = new_y;
        }

        glm::mat4 trans = glm::rotate(glm::mat4(1.0f),
                                      glm::radians(total_elapsed_sec * 120.0f),
                                      rotation_axis);
        surface->set_transformation(trans);

        float const alpha_amplitude = (1.0f - min_alpha) / 2.0f;
        surface->set_alpha(min_alpha + alpha_amplitude +
                           alpha_amplitude *
                           sin(alpha_offset + 2 * M_PI * total_elapsed_sec /
                               3.0));
    }

private:
    std::shared_ptr<ms::Surface> surface;
    geom::Size display_size;
    float x;
    float y;
    float w;
    float h;
    float dx;
    float dy;
    StopWatch stop_watch;
    glm::vec3 rotation_axis;
    float alpha_offset;
};
///\internal [Moveable_tag]

void callback_when_started(mir::Server& server, std::function<void()> callback)
{
    struct ServerStatusListener : mir::ServerStatusListener
    {
        ServerStatusListener(std::function<void()> callback) :
            callback(callback) {}

        virtual void paused() override {}
        virtual void resumed() override {}
        virtual void started() override {callback(); callback = []{}; }

        std::function<void()> callback;
    };

    server.override_the_server_status_listener([callback]
        {
            return std::make_shared<ServerStatusListener>(callback);
        });
}

class RenderSurfacesDisplayBufferCompositor : public mc::DisplayBufferCompositor
{
public:
    RenderSurfacesDisplayBufferCompositor(
        std::unique_ptr<DisplayBufferCompositor> db_compositor,
        std::vector<Moveable>& moveables)
        : db_compositor{std::move(db_compositor)},
          moveables(moveables),
          frames{0}
    {
    }

    void composite(mc::SceneElementSequence&& scene_sequence) override
    {
        while (!created) std::this_thread::yield();
        stop_watch.stop();
        if (stop_watch.elapsed_seconds_since_last_restart() >= 1)
        {
            std::cout << "FPS: " << frames << " Frame Time: " << 1.0 / frames << std::endl;
            frames = 0;
            stop_watch.restart();
        }

        db_compositor->composite(std::move(scene_sequence));

        for (auto& m : moveables)
            m.step();

        frames++;
    }

private:
    std::unique_ptr<DisplayBufferCompositor> const db_compositor;
    StopWatch stop_watch;
    std::vector<Moveable>& moveables;
    uint32_t frames;
};

class RenderSurfacesDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    RenderSurfacesDisplayBufferCompositorFactory(
        std::shared_ptr<mc::DisplayBufferCompositorFactory> const& factory,
        std::vector<Moveable>& moveables)
        : factory{factory},
          moveables(moveables)
    {
    }

    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& display_buffer)
    {
        auto compositor = factory->create_compositor_for(display_buffer);
        auto raw = new RenderSurfacesDisplayBufferCompositor(
            std::move(compositor), moveables);
        return std::unique_ptr<RenderSurfacesDisplayBufferCompositor>(raw);
    }

private:
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const factory;
    std::vector<Moveable>& moveables;
};

///\internal [RenderSurfacesServer_tag]
// Extend the default configuration to manage moveables.
class RenderSurfacesServer : private mir::Server
{
    std::shared_ptr<mir::input::EventFilter> const quit_filter{me::make_quit_filter_for(*this)};
public:
    RenderSurfacesServer(int argc, char const** argv)
    {
        me::add_display_configuration_options_to(*this);

        ///\internal [RenderSurfacesDisplayBufferCompositor_tag]
        // Decorate the DefaultDisplayBufferCompositor in order to move surfaces.
        wrap_display_buffer_compositor_factory([this](std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped)
            {
                return std::make_shared<RenderSurfacesDisplayBufferCompositorFactory>(
                    wrapped,
                    moveables);
            });
        ///\internal [RenderSurfacesDisplayBufferCompositor_tag]

        add_configuration_option(surfaces_to_render, "Number of surfaces to render", 5);

        // Provide the command line and run the *this
        set_command_line(argc, argv);
        setenv("MIR_SERVER_NO_FILE", "", 1);

        // Unless the compositor starts before we create the surfaces it won't respond to
        // the change notification that causes.
        callback_when_started(*this, [this] { create_surfaces(); });

        apply_settings();
    }

    using mir::Server::run;
    using mir::Server::exited_normally;

    // New function to initialize moveables with surfaces
    void create_surfaces()
    {
        moveables.resize(get_options()->get<int>(surfaces_to_render));
        std::cout << "Rendering " << moveables.size() << " surfaces" << std::endl;

        auto const display = the_display();
        auto const buffer_stream_factory = the_buffer_stream_factory();
        auto const surface_factory = the_surface_factory();
        auto const surface_coordinator = the_surface_coordinator();
        auto const gl_context = the_display()->create_gl_context();

        /* TODO: Get proper configuration */
        geom::Rectangles view_area;
        display->for_each_display_sync_group([&](mg::DisplaySyncGroup& group)
        {
            group.for_each_display_buffer([&](mg::DisplayBuffer& db)
            {
                view_area.add(db.view_area());
            });
        });
        geom::Size const display_size{view_area.bounding_rectangle().size};
        uint32_t const surface_side{300};
        geom::Size const surface_size{surface_side, surface_side};

        float const angular_step = 2.0 * M_PI / moveables.size();
        float const w = display_size.width.as_uint32_t();
        float const h = display_size.height.as_uint32_t();
        auto const surface_pf = supported_pixel_formats()[0];

        int i = 0;
        for (auto& m : moveables)
        {
            auto params = ms::a_surface()
                .of_size(surface_size)
                .of_pixel_format(surface_pf)
                .of_buffer_usage(mg::BufferUsage::hardware);
            mg::BufferProperties properties{params.size, params.pixel_format, params.buffer_usage};
            struct NullBufferSink : mf::BufferSink
            {
                void send_buffer(mf::BufferStreamId, mg::Buffer&, mg::BufferIpcMsgType) override {}
            };

            auto const stream = buffer_stream_factory->create_buffer_stream(
                mf::BufferStreamId{}, std::make_shared<NullBufferSink>(), properties);
            auto const surface = surface_factory->create_surface(stream, params);
            surface_coordinator->add_surface(surface, params.depth, params.input_mode, nullptr);

            {
                mg::Buffer* buffer{nullptr};
                auto const complete = [&](mg::Buffer* new_buf){ buffer = new_buf; };
                surface->primary_buffer_stream()->swap_buffers(buffer, complete); // Fetch buffer for rendering
                {
                    gl_context->make_current();

                    mt::ImageRenderer img_renderer{mir_image.pixel_data,
                                   geom::Size{mir_image.width, mir_image.height},
                                   mir_image.bytes_per_pixel};
                    mt::BufferRenderTarget brt{*buffer};
                    brt.make_current();
                    img_renderer.render();

                    gl_context->release_current();
                }
                surface->primary_buffer_stream()->swap_buffers(buffer, complete); // Post rendered buffer
            }

            /*
             * Place each surface at a different starting location and give it a
             * different speed, rotation and alpha offset.
             */
            uint32_t const x = w * (0.5 + 0.25 * cos(i * angular_step)) - surface_side / 2.0;
            uint32_t const y = h * (0.5 + 0.25 * sin(i * angular_step)) - surface_side / 2.0;

            surface->move_to({x, y});
            m = Moveable(surface, display_size,
                    cos(0.1f + i * M_PI / 6.0f) * w / 3.0f,
                    sin(0.1f + i * M_PI / 6.0f) * h / 3.0f,
                    glm::vec3{(i % 3 == 0) * 1.0f, (i % 3 == 1) * 1.0f, (i % 3 == 2) * 1.0f},
                    2.0f * M_PI * cos(i));
            ++i;
        }

        created = true;
    }

private:
    std::vector<Moveable> moveables;
};
///\internal [RenderSurfacesServer_tag]
}

int main(int argc, char const** argv)
try
{
    ///\internal [main_tag]
    RenderSurfacesServer render_surfaces{argc, argv};

    render_surfaces.run();

    return render_surfaces.exited_normally() ? EXIT_SUCCESS : EXIT_FAILURE;
}
catch (...)
{
    mir::report_exception();
    return EXIT_FAILURE;
}
