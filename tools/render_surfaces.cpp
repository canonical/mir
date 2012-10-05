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

#include "mir/graphics/platform.h"
#include "mir/graphics/display.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/compositor.h"
#include "mir/graphics/gl_renderer.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/geometry/rectangle.h"
#include "mir/chrono/chrono.h"
#include <csignal>
#include <iostream>
#include <sstream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace mg=mir::graphics;
namespace mc=mir::compositor;
namespace ms=mir::surfaces;
namespace geom=mir::geometry;

using namespace std;

#ifndef ANDROID
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::microseconds microsec;
#endif

volatile std::sig_atomic_t running = true;

void signal_handler(int /*signum*/)
{
    running = false;
}

struct Moveable
{
    Moveable() {}
    Moveable(ms::Surface& s, const geom::Size& display_size,
             float dx, float dy, const glm::vec3& rotation_axis, float alpha_offset)
        : surface(&s), display_size(display_size),
          x{static_cast<float>(s.top_left().x.as_uint32_t())},
          y{static_cast<float>(s.top_left().y.as_uint32_t())},
          w{static_cast<float>(s.size().width.as_uint32_t())},
          h{static_cast<float>(s.size().height.as_uint32_t())},
          dx{dx}, dy{dy},
#ifndef ANDROID
          start{Clock::now()}, last_update{start},
#else
          step_count{0},
#endif
          rotation_axis{rotation_axis}, alpha_offset{alpha_offset}
    {
    }

    void step()
    {
#ifndef ANDROID
        Clock::time_point now = Clock::now();
        microsec elapsed = std::chrono::duration_cast<microsec>(last_update - now);
        float elapsed_sec = elapsed.count() / 1000000.0;
        microsec total_elapsed = std::chrono::duration_cast<microsec>(now - start);
        float total_elapsed_sec = total_elapsed.count() / 1000000.0;
        last_update = now;
#else
        float elapsed_sec = 1.0f / 60.0f;
        float total_elapsed_sec = step_count * 1.0f / 60.0f;
        ++step_count;
#endif

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

    ms::Surface* surface;
    geom::Size display_size;
    float x;
    float y;
    float w;
    float h;
    float dx;
    float dy;
#ifndef ANDROID
    Clock::time_point start;
    Clock::time_point last_update;
#else
    uint32_t step_count;
#endif
    glm::vec3 rotation_axis;
    float alpha_offset;
};

int main(int argc, char **argv)
{
    /* Create and set up all the components we need */
    auto platform = mg::create_platform();
    auto display = platform->create_display();
    const geom::Size display_size = display->view_area().size;
    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto buffer_allocator = platform->create_buffer_allocator(buffer_initializer);
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(buffer_allocator);
    mc::BufferBundleManager manager{strategy};
    ms::SurfaceStack surface_stack{&manager};
    auto gl_renderer = std::make_shared<mg::GLRenderer>(display_size);
    mc::Compositor compositor{&surface_stack,gl_renderer};
    
    /* Set up graceful exit on SIGINT */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);  

    /* Parse the command line */
    int num_moveables = 5;

    if (argc > 1)
    {
        std::stringstream ss{argv[1]};
        ss >> num_moveables;
    }

    std::cout << "Rendering " << num_moveables << " surfaces" << std::endl;

    /* Set up the surfaces */
    std::vector<Moveable> m{num_moveables};

    const uint32_t surface_size{300};

    for (int i = 0; i < num_moveables; ++i)
    {
        const float w = display_size.width.as_uint32_t();
        const float h = display_size.height.as_uint32_t();
        const float angular_step = 2.0 * M_PI / num_moveables;

        std::shared_ptr<ms::Surface> s = surface_stack.create_surface(
            ms::a_surface().of_size({geom::Width{surface_size}, geom::Height{surface_size}})).lock();

        /* 
         * Place each surface at a different starting location and give it a
         * different speed, rotation and alpha offset.
         */
        uint32_t x = w * (0.5 + 0.25 * cos(i * angular_step)) - surface_size / 2.0;
        uint32_t y = h * (0.5 + 0.25 * sin(i * angular_step)) - surface_size / 2.0;

        s->move_to({geom::X{x}, geom::Y{y}});
        m[i] = Moveable(*s, display_size,
                              cos(0.1f + i * M_PI / 6.0f) * w / 3.0f,
                              sin(0.1f + i * M_PI / 6.0f) * h / 3.0f,
                              glm::vec3{(i % 3 == 0) * 1.0f, (i % 3 == 1) * 1.0f, (i % 3 == 2) * 1.0f},
                              2.0f * M_PI * cos(i));
    }

    /* Draw! */
    glClearColor(0.0, 1.0, 0.0, 1.0);
    uint32_t frames = 0;

#ifndef ANDROID
    Clock::time_point t0 = Clock::now();
#endif
    while (running) {
#ifndef ANDROID
        Clock::time_point t1 = Clock::now();
        if (std::chrono::duration_cast<microsec>(t1 - t0).count() >= 1000000) {
            std::cout << "FPS: " << frames << " Frame Time: " << 1.0 / frames << std::endl;
            frames = 0;
            t0 = t1;
        }
#endif

        /* Update surface state */
        for (int i = 0; i < num_moveables; ++i)
            m[i].step();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        compositor.render(display.get());

        frames++;
    }

    return 0;
}
