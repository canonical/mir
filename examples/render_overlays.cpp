/*
 * Copyright © 2012, 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/server.h"
#include "mir/report_exception.h"
#include "mir/graphics/display.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/renderer/gl/render_target.h"
#include "mir_image.h"
#include "as_render_target.h"

#include <chrono>
#include <csignal>
#include <iostream>

namespace mg=mir::graphics;
namespace mo=mir::options;
namespace geom=mir::geometry;
namespace me=mir::examples;

namespace
{
volatile std::sig_atomic_t running = true;

void signal_handler(int /*signum*/)
{
    running = false;
}

class PixelBufferABGR
{
public:
    PixelBufferABGR(geom::Size sz, uint32_t color) :
        size{sz.width.as_uint32_t() * sz.height.as_uint32_t()},
        data{new uint32_t[size]}
    {
        fill(color);
    }

    void fill(uint32_t color)
    {
        for(auto i = 0u; i < size; i++)
            data[i] = color;
    }

    unsigned char* pixels()
    {
        return reinterpret_cast<unsigned char*>(data.get());
    }

    size_t pixel_size()
    {
        return size * sizeof(uint32_t);
    }
private:
    size_t size;
    std::unique_ptr<uint32_t[]> data;
};

class DemoOverlayClient
{
public:
    DemoOverlayClient(
        mg::GraphicBufferAllocator& buffer_allocator,
        mg::BufferProperties const& buffer_properties, uint32_t color)
         : front_buffer(buffer_allocator.alloc_buffer(buffer_properties)),
           back_buffer(buffer_allocator.alloc_buffer(buffer_properties)),
           color{color},
           last_tick{std::chrono::high_resolution_clock::now()},
           pixel_buffer{buffer_properties.size, color}
    {
    }

    void update_green_channel()
    {
        char green_value = (color >> 8) & 0xFF;
        green_value += compute_update_value();
        color &= 0xFFFF00FF;
        color |= (green_value << 8);
        pixel_buffer.fill(color);

        back_buffer->write(pixel_buffer.pixels(), pixel_buffer.pixel_size());
        std::swap(front_buffer, back_buffer);
    }

    std::shared_ptr<mg::Buffer> last_rendered()
    {
        return front_buffer;
    }

private:
    int compute_update_value()
    {
        float const update_ratio{3.90625}; //this will give an update of 256 in 1s  
        auto current_tick = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_tick - last_tick).count();
        float update_value = elapsed_ms / update_ratio;
        last_tick = current_tick;
        return static_cast<int>(update_value);
    }

    std::shared_ptr<mg::Buffer> front_buffer;
    std::shared_ptr<mg::Buffer> back_buffer;
    unsigned int color;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick;
    PixelBufferABGR pixel_buffer;
};

class DemoRenderable : public mg::Renderable
{
public:
    DemoRenderable(std::shared_ptr<DemoOverlayClient> const& client, geom::Rectangle rect)
        : client(client),
          position(rect)
    {
    }

    ID id() const override
    {
        return this;
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return client->last_rendered();
    }

    geom::Rectangle screen_position() const override
    {
        return position;
    }

    float alpha() const override
    {
        return 1.0f;
    }

    glm::mat4 transformation() const override
    {
        return trans;
    }

    bool shaped() const override
    {
        return false;
    }

private:
    std::shared_ptr<DemoOverlayClient> const client;
    geom::Rectangle const position;
    glm::mat4 const trans;
};

void render_loop(mir::Server& server)
{
    /* Set up graceful exit on SIGINT and SIGTERM */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    auto platform = server.the_graphics_platform();
    auto display = server.the_display();
    auto buffer_allocator = platform->create_buffer_allocator();

    mg::BufferProperties buffer_properties{
        geom::Size{512, 512},
        mir_pixel_format_abgr_8888,
        mg::BufferUsage::hardware
    };

    auto client1 = std::make_shared<DemoOverlayClient>(
        *buffer_allocator, buffer_properties, 0xFF0000FF);
    auto client2 = std::make_shared<DemoOverlayClient>(
        *buffer_allocator, buffer_properties, 0xFFFFFF00);

    mg::RenderableList renderlist
    {
        std::make_shared<DemoRenderable>(client1, geom::Rectangle{{0,0} , buffer_properties.size}),
        std::make_shared<DemoRenderable>(client2, geom::Rectangle{{80,80} , buffer_properties.size})
    };

    while (running)
    {
        client1->update_green_channel();
        client2->update_green_channel();
        display->for_each_display_sync_group([&](mg::DisplaySyncGroup& group)
        {
            group.for_each_display_buffer([&](mg::DisplayBuffer& buffer)
            {
                // TODO: Is make_current() really needed here?
                me::as_render_target(buffer)->make_current();
                buffer.post_renderables_if_optimizable(renderlist);
            });
            group.post();
        });
    }
}
}

int main(int argc, char const** argv)
try
{
    mir::Server server;
    server.set_command_line(argc, argv);
    server.apply_settings();

    render_loop(server);

    return EXIT_SUCCESS;
}
catch (...)
{
    mir::report_exception(std::cerr);
    return EXIT_FAILURE;
}
