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

#include "mir/default_server_configuration.h"
#include "mir/graphics/display.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/report_exception.h"

#include "graphics_region_factory.h"
#include "patterns.h"

#include <csignal>
#include <iostream>

namespace mg=mir::graphics;
namespace ml=mir::logging;
namespace mo=mir::options;
namespace geom=mir::geometry;

namespace
{
volatile std::sig_atomic_t running = true;

void signal_handler(int /*signum*/)
{
    running = false;
}

class DemoOverlayClient
{
public:
    DemoOverlayClient(
        mg::GraphicBufferAllocator& buffer_allocator,
        mg::BufferProperties const& buffer_properties, uint32_t color, uint32_t c2)
         : front_buffer(buffer_allocator.alloc_buffer(buffer_properties)),
           back_buffer(buffer_allocator.alloc_buffer(buffer_properties)),
           region_factory(mir::test::draw::create_graphics_region_factory()),
           fill{color},
           fill2{c2}, count{0}
    {
    }

    void draw()
    {
        if (count++ %2)
            fill.draw(
                *region_factory->graphic_region_from_handle(*back_buffer->native_buffer_handle()));
        else
            fill2.draw(
                *region_factory->graphic_region_from_handle(*back_buffer->native_buffer_handle()));

        std::swap(front_buffer, back_buffer);
    }

    std::shared_ptr<mg::Buffer> last_rendered()
    {
        return front_buffer;
    }

private:
    std::shared_ptr<mg::Buffer> front_buffer;
    std::shared_ptr<mg::Buffer> back_buffer;
    std::shared_ptr<mir::test::draw::GraphicsRegionFactory> region_factory;
    mir::test::draw::DrawPatternSolid fill;
    mir::test::draw::DrawPatternSolid fill2;
    int count;
};

class DemoRenderable : public mg::Renderable
{
public:
    DemoRenderable(std::shared_ptr<DemoOverlayClient> const& client, geom::Rectangle rect)
        : client(std::move(client)),
          position(rect)
    {
    }

    std::shared_ptr<mg::Buffer> buffer() const
    {
        return client->last_rendered();
    }

    bool alpha_enabled() const
    {
        return false;
    }

    geom::Rectangle screen_position() const
    {
        return position;
    }

private:
    std::shared_ptr<DemoOverlayClient> const client;
    geom::Rectangle const position;
};
}

int main(int argc, char const** argv)
try
{

    /* Set up graceful exit on SIGINT and SIGTERM */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    mir::DefaultServerConfiguration conf{argc, argv};

    auto platform = conf.the_graphics_platform();
    auto display = platform->create_display(conf.the_display_configuration_policy());
    auto buffer_allocator = platform->create_buffer_allocator(conf.the_buffer_initializer());

     mg::BufferProperties buffer_properties{
        geom::Size{512, 512},
        mir_pixel_format_abgr_8888,
        mg::BufferUsage::hardware
    };

    auto client1 = std::make_shared<DemoOverlayClient>(*buffer_allocator, buffer_properties,0xFF00FF00, 0xFFFFFFFF);
    auto client2 = std::make_shared<DemoOverlayClient>(*buffer_allocator, buffer_properties,0xFFFF0000, 0xFFFFFFFF);

    std::list<std::shared_ptr<mg::Renderable>> renderlist
    {
        std::make_shared<DemoRenderable>(client1, geom::Rectangle{{0,0} , {512, 512}}),
        std::make_shared<DemoRenderable>(client2, geom::Rectangle{{80,80} , {592,592}})
    };

    while (running)
    {
        display->for_each_display_buffer([&](mg::DisplayBuffer& buffer)
        {
            buffer.make_current();
            client1->draw();
            client2->draw();
            auto render_fn = [](mg::Renderable const&) {};
            buffer.render_and_post_update(renderlist, render_fn);
        });
    }
   return 0;
}
catch (...)
{
    mir::report_exception(std::cerr);
    return 1;
}
