/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/connector.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangles.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/shell/surface_factory.h"
#include "mir/shell/surface.h"
#include "mir/run_mir.h"
#include "mir/report_exception.h"

#include "mir_image.h"
#include "buffer_render_target.h"
#include "image_renderer.h"
#include "server_configuration.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <sstream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mf = mir::frontend;
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
/// it also provides a bespoke buffer initializer
/// \snippet render_surfaces.cpp RenderResourcesBufferInitializer_tag
/// and a bespoke display buffer compositor
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
bool input_is_on = false;
std::weak_ptr<mg::Cursor> cursor;
static const uint32_t bg_color = 0x00000000;
static const uint32_t fg_color = 0xffdd4814;

void update_cursor(uint32_t bg_color, uint32_t fg_color)
{
    if (auto cursor = ::cursor.lock())
    {
        static const int width = 64;
        static const int height = 64;
        std::vector<uint32_t> image(height * width, bg_color);
        for (int i = 0; i != width-1; ++i)
        {
            if (i < 16)
            {
                image[0 * height + i] = fg_color;
                image[1 * height + i] = fg_color;
                image[i * height + 0] = fg_color;
                image[i * height + 1] = fg_color;
            }
            image[i * height + i] = fg_color;
            image[(i+1) * height + i] = fg_color;
            image[i * height + i + 1] = fg_color;
        }
        cursor->set_image(image.data(), geom::Size{width, height});
    }
}

void animate_cursor()
{
    if (!input_is_on)
    {
        if (auto cursor = ::cursor.lock())
        {
            static int cursor_pos = 0;
            if (++cursor_pos == 300)
            {
                cursor_pos = 0;

                static const uint32_t fg_colors[3] = { fg_color, 0xffffffff, 0x3f000000 };
                static int fg_color = 0;

                if (++fg_color == 3) fg_color = 0;

                update_cursor(bg_color, fg_colors[fg_color]);
            }

            cursor->move_to(geom::Point{cursor_pos, cursor_pos});
        }
    }
}

char const* const surfaces_to_render = "surfaces-to-render";
char const* const display_cursor     = "display-cursor";

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
    Moveable(std::shared_ptr<msh::Surface> const& s, const geom::Size& display_size,
             float dx, float dy, const glm::vec3& rotation_axis, float alpha_offset)
        : surface(s), display_size(display_size),
          x{static_cast<float>(s->top_left().x.as_uint32_t())},
          y{static_cast<float>(s->top_left().y.as_uint32_t())},
          w{static_cast<float>(s->size().width.as_uint32_t())},
          h{static_cast<float>(s->size().height.as_uint32_t())},
          dx{dx},
          dy{dy},
          rotation_axis{rotation_axis},
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

        surface->set_rotation(total_elapsed_sec * 120.0f, rotation_axis);
        surface->set_alpha(0.5 + 0.5 * sin(alpha_offset + 2 * M_PI * total_elapsed_sec / 3.0));
    }

private:
    std::shared_ptr<msh::Surface> surface;
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

///\internal [RenderSurfacesServerConfiguration_tag]
// Extend the default configuration to manage moveables.
class RenderSurfacesServerConfiguration : public me::ServerConfiguration
{
public:
    RenderSurfacesServerConfiguration(int argc, char const** argv)
        : ServerConfiguration(argc, argv)
    {
        namespace po = boost::program_options;

        add_options()
            (surfaces_to_render, po::value<int>(),  "Number of surfaces to render"
                                                    " [int:default=5]")
            (display_cursor, po::value<bool>(), "Display test cursor. (If input is "
                                                "disabled it gets animated.) "
                                                "[bool:default=false]");
    }

    ///\internal [RenderSurfacesServerConfiguration_stubs_tag]
    // Stub out server connectivity.
    std::shared_ptr<mf::Connector> the_connector() override
    {
        struct NullConnector : public mf::Connector
        {
            void start() {}
            void stop() {}
            int client_socket_fd() const override { return 0; }
            void remove_endpoint() const override { }
        };

        return std::make_shared<NullConnector>();
    }
    ///\internal [RenderSurfacesServerConfiguration_stubs_tag]

    ///\internal [RenderResourcesBufferInitializer_tag]
    // Override for a bespoke buffer initializer
    std::shared_ptr<mg::BufferInitializer> the_buffer_initializer() override
    {
        class RenderResourcesBufferInitializer : public mg::BufferInitializer
        {
        public:
            RenderResourcesBufferInitializer()
            {
            }

            void operator()(mg::Buffer& buffer)
            {
                mt::ImageRenderer img_renderer{mir_image.pixel_data,
                               geom::Size{mir_image.width, mir_image.height},
                               mir_image.bytes_per_pixel};
                mt::BufferRenderTarget brt{buffer};
                brt.make_current();
                img_renderer.render();
            }
        };

        return std::make_shared<RenderResourcesBufferInitializer>();
    }
    ///\internal [RenderResourcesBufferInitializer_tag]

    ///\internal [RenderSurfacesDisplayBufferCompositor_tag]
    // Decorate the DefaultDisplayBufferCompositor in order to move surfaces.
    std::shared_ptr<mc::DisplayBufferCompositorFactory> the_display_buffer_compositor_factory() override
    {
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

            void composite()
            {
                animate_cursor();
                stop_watch.stop();
                if (stop_watch.elapsed_seconds_since_last_restart() >= 1)
                {
                    std::cout << "FPS: " << frames << " Frame Time: " << 1.0 / frames << std::endl;
                    frames = 0;
                    stop_watch.restart();
                }

                glClearColor(0.0, 1.0, 0.0, 1.0);
                db_compositor->composite();

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

        return std::make_shared<RenderSurfacesDisplayBufferCompositorFactory>(
            me::ServerConfiguration::the_display_buffer_compositor_factory(),
            moveables);
    }
    ///\internal [RenderSurfacesDisplayBufferCompositor_tag]

    // New function to initialize moveables with surfaces
    void create_surfaces()
    {
        moveables.resize(the_options()->get(surfaces_to_render, 5));
        std::cout << "Rendering " << moveables.size() << " surfaces" << std::endl;

        auto const display = the_display();
        auto const surface_factory = the_scene_surface_factory();
        /* TODO: Get proper configuration */
        geom::Rectangles view_area;
        display->for_each_display_buffer([&view_area](mg::DisplayBuffer const& db)
        {
            view_area.add(db.view_area());
        });
        geom::Size const display_size{view_area.bounding_rectangle().size};
        uint32_t const surface_side{300};
        geom::Size const surface_size{surface_side, surface_side};

        float const angular_step = 2.0 * M_PI / moveables.size();
        float const w = display_size.width.as_uint32_t();
        float const h = display_size.height.as_uint32_t();
        auto const surface_pf = the_buffer_allocator()->supported_pixel_formats()[0];

        int i = 0;
        for (auto& m : moveables)
        {
            auto const s = surface_factory->create_surface(
                    nullptr,
                    msh::a_surface().of_size(surface_size)
                                   .of_pixel_format(surface_pf)
                                   .of_buffer_usage(mg::BufferUsage::hardware),
                    mf::SurfaceId(), {});

            /*
             * We call swap_buffers() twice so that the surface is
             * considers the first buffer to be posted.
             * (TODO There must be a better way!)
             */
            {
                std::shared_ptr<mg::Buffer> tmp;
                s->swap_buffers(tmp);
                s->swap_buffers(tmp);
            }

            /*
             * Place each surface at a different starting location and give it a
             * different speed, rotation and alpha offset.
             */
            uint32_t const x = w * (0.5 + 0.25 * cos(i * angular_step)) - surface_side / 2.0;
            uint32_t const y = h * (0.5 + 0.25 * sin(i * angular_step)) - surface_side / 2.0;

            s->move_to({x, y});
            m = Moveable(s, display_size,
                    cos(0.1f + i * M_PI / 6.0f) * w / 3.0f,
                    sin(0.1f + i * M_PI / 6.0f) * h / 3.0f,
                    glm::vec3{(i % 3 == 0) * 1.0f, (i % 3 == 1) * 1.0f, (i % 3 == 2) * 1.0f},
                    2.0f * M_PI * cos(i));
            ++i;
        }
    }

    bool input_is_on()
    {
        return the_options()->get("enable-input", ::input_is_on);
    }

    std::weak_ptr<mg::Cursor> the_cursor()
    {
        if (the_options()->get(display_cursor, false))
        {
            return the_display()->the_cursor();
        }
        else
        {
            return {};
        }
    }

private:
    std::vector<Moveable> moveables;
};
///\internal [RenderSurfacesServerConfiguration_tag]
}

int main(int argc, char const** argv)
try
{
    ///\internal [main_tag]
    RenderSurfacesServerConfiguration conf{argc, argv};

    mir::run_mir(conf, [&](mir::DisplayServer&)
    {
        conf.create_surfaces();

        cursor = conf.the_cursor();

        input_is_on = conf.input_is_on();
    });
    ///\internal [main_tag]

    return 0;
}
catch (...)
{
    mir::report_exception(std::cerr);
    return 1;
}
