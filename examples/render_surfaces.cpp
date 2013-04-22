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

#include "mir/compositor/default_compositing_strategy.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/frontend/communicator.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/display.h"
#include "mir/input/null_input_manager.h"
#include "mir/shell/surface_builder.h"
#include "mir/surfaces/surface.h"
#include "mir/default_server_configuration.h"
#include "mir/run_mir.h"
#include "mir/abnormal_exit.h"

#include "mir_image.h"
#include "buffer_render_target.h"
#include "image_renderer.h"

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
namespace ms = mir::surfaces;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::tools;

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
/// and a bespoke compositing strategy
/// \snippet render_surfaces.cpp RenderSurfacesCompositingStrategy_tag
///\section Utilities Utility classes
/// For smooth animation we need to track time and move surfaces accordingly
///\subsection StopWatch StopWatch
/// \snippet render_surfaces.cpp StopWatch_tag
///\subsection Moveable Moveable
/// \snippet render_surfaces.cpp Moveable_tag

///\example render_surfaces.cpp A simple program using the mir library.

namespace
{
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
    Moveable(ms::Surface& s, const geom::Size& display_size,
             float dx, float dy, const glm::vec3& rotation_axis, float alpha_offset)
        : surface(&s), display_size(display_size),
          x{static_cast<float>(s.top_left().x.as_uint32_t())},
          y{static_cast<float>(s.top_left().y.as_uint32_t())},
          w{static_cast<float>(s.size().width.as_uint32_t())},
          h{static_cast<float>(s.size().height.as_uint32_t())},
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
            surface->move_to({geom::X{static_cast<uint32_t>(new_x)},
                              geom::Y{static_cast<uint32_t>(new_y)}});
            x = new_x;
            y = new_y;
        }

        surface->set_rotation(total_elapsed_sec * 120.0f, rotation_axis);
        surface->set_alpha(0.5 + 0.5 * sin(alpha_offset + 2 * M_PI * total_elapsed_sec / 3.0));
    }

private:
    ms::Surface* surface;
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
class RenderSurfacesServerConfiguration : public mir::DefaultServerConfiguration
{
public:
    RenderSurfacesServerConfiguration(unsigned int num_moveables)
        : mir::DefaultServerConfiguration{0, nullptr},
          moveables{num_moveables}
    {
    }

    ///\internal [RenderSurfacesServerConfiguration_stubs_tag]
    // Stub out server connectivity.
    std::shared_ptr<mf::Communicator> the_communicator() override
    {
        struct NullCommunicator : public mf::Communicator
        {
            void start() {}
            void stop() {}
        };

        return std::make_shared<NullCommunicator>();
    }

    // Stub out input.
    std::shared_ptr<mi::InputManager> the_input_manager() override
    {
        return std::make_shared<mi::NullInputManager>();
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
                : img_renderer{mir_image.pixel_data,
                               geom::Size{geom::Width{mir_image.width},
                                          geom::Height{mir_image.height}},
                               mir_image.bytes_per_pixel}
            {
            }

            void operator()(mc::Buffer& buffer)
            {
                mt::BufferRenderTarget brt{buffer};
                brt.make_current();
                img_renderer.render();
            }

        private:
            mt::ImageRenderer img_renderer;
        };

        return std::make_shared<RenderResourcesBufferInitializer>();
    }
    ///\internal [RenderResourcesBufferInitializer_tag]

    ///\internal [RenderSurfacesCompositingStrategy_tag]
    // Decorate the DefaultCompositingStrategy in order to move surfaces.
    std::shared_ptr<mc::CompositingStrategy> the_compositing_strategy() override
    {
        class RenderSurfacesCompositingStrategy : public mc::CompositingStrategy
        {
        public:
            RenderSurfacesCompositingStrategy(std::shared_ptr<mc::Renderables> const& renderables,
                                              std::shared_ptr<mg::Renderer> const& renderer,
                                              std::vector<Moveable>& moveables)
                : default_compositing_strategy{renderables, renderer},
                  frames{0},
                  moveables(moveables)
            {
            }

            void render(mg::DisplayBuffer& display_buffer)
            {
                stop_watch.stop();
                if (stop_watch.elapsed_seconds_since_last_restart() >= 1)
                {
                    std::cout << "FPS: " << frames << " Frame Time: " << 1.0 / frames << std::endl;
                    frames = 0;
                    stop_watch.restart();
                }

                glClearColor(0.0, 1.0, 0.0, 1.0);
                default_compositing_strategy.render(display_buffer);

                for (auto& m : moveables)
                    m.step();

                frames++;
            }

        private:
            mc::DefaultCompositingStrategy default_compositing_strategy;
            StopWatch stop_watch;
            uint32_t frames;
            std::vector<Moveable>& moveables;
        };

        return std::make_shared<RenderSurfacesCompositingStrategy>(the_renderables(),
                                                                   the_renderer(),
                                                                   moveables);
    }
    ///\internal [RenderSurfacesCompositingStrategy_tag]

    // New function to initialize moveables with surfaces
    void create_surfaces()
    {
        auto const display = the_display();
        auto const surface_builder = the_surface_builder();
        geom::Size const display_size{display->view_area().size};
        uint32_t const surface_side{300};
        geom::Size const surface_size{geom::Width{surface_side},
                                      geom::Height{surface_side}};

        float const angular_step = 2.0 * M_PI / moveables.size();
        float const w = display_size.width.as_uint32_t();
        float const h = display_size.height.as_uint32_t();
        auto const surface_pf = the_buffer_allocator()->supported_pixel_formats()[0];

        int i = 0;
        for (auto& m : moveables)
        {
            std::shared_ptr<ms::Surface> s = surface_builder->create_surface(
                    mf::a_surface().of_size(surface_size)
                                   .of_pixel_format(surface_pf)
                                   .of_buffer_usage(mc::BufferUsage::hardware)
                    ).lock();

            /*
             * Place each surface at a different starting location and give it a
             * different speed, rotation and alpha offset.
             */
            uint32_t const x = w * (0.5 + 0.25 * cos(i * angular_step)) - surface_side / 2.0;
            uint32_t const y = h * (0.5 + 0.25 * sin(i * angular_step)) - surface_side / 2.0;

            s->move_to({geom::X{x}, geom::Y{y}});
            m = Moveable(*s, display_size,
                    cos(0.1f + i * M_PI / 6.0f) * w / 3.0f,
                    sin(0.1f + i * M_PI / 6.0f) * h / 3.0f,
                    glm::vec3{(i % 3 == 0) * 1.0f, (i % 3 == 1) * 1.0f, (i % 3 == 2) * 1.0f},
                    2.0f * M_PI * cos(i));
            ++i;
        }
    }

private:
    std::vector<Moveable> moveables;
};
///\internal [RenderSurfacesServerConfiguration_tag]
}

int main(int argc, char **argv)
try
{
    ///\internal [main_tag]
    /* Parse the command line */
    unsigned int num_moveables{5};

    if (argc > 1)
    {
        std::stringstream ss{argv[1]};
        ss >> num_moveables;
    }

    std::cout << "Rendering " << num_moveables << " surfaces" << std::endl;

    RenderSurfacesServerConfiguration conf{num_moveables};

    mir::run_mir(conf, [&](mir::DisplayServer&) {conf.create_surfaces();});
    ///\internal [main_tag]

    return 0;
}
catch (mir::AbnormalExit const& error)
{
    std::cerr << error.what() << std::endl;
    return 1;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
}
