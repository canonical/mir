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

#include "testdraw/graphics_region_factory.h"
#include "testdraw/patterns.h"

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
}

class DemoRenderable : public mg::Renderable
{
public:
    DemoRenderable(std::shared_ptr<mg::Buffer> const& buffer, geom::Rectangle rect)
        : renderable_buffer(buffer),
          position(rect)
    {
    }

    std::shared_ptr<mg::Buffer> buffer(unsigned long) const override
    {
        return renderable_buffer;
    }

    bool alpha_enabled() const
    {
        return false;
    }

    geom::Rectangle screen_position() const
    {
        return position;
    }

    float alpha() const override
    {
        return 1.0f;
    }

    glm::mat4 const& transformation() const override
    {
        return trans;
    }

    bool shaped() const
    {
        return false;
    }

    bool should_be_rendered_in(geom::Rectangle const& rect) const override
    {
        return rect.overlaps(position);
    }

private:
    std::shared_ptr<mg::Buffer> const renderable_buffer;
    geom::Rectangle const position;
    glm::mat4 const trans;
};

int main(int argc, char const** argv)
try
{
    mir::test::draw::DrawPatternSolid fill_with_green(0x00FF00FF);
    mir::test::draw::DrawPatternSolid fill_with_blue(0x0000FFFF);

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
    auto region_factory = mir::test::draw::create_graphics_region_factory();

     mg::BufferProperties buffer_properties{
        geom::Size{512, 512},
        mir_pixel_format_abgr_8888,
        mg::BufferUsage::hardware
    };

    auto buffer1 = buffer_allocator->alloc_buffer(buffer_properties);
    auto buffer2 = buffer_allocator->alloc_buffer(buffer_properties);

    fill_with_green.draw(*region_factory->graphic_region_from_handle(*buffer1->native_buffer_handle()));
    fill_with_blue.draw(*region_factory->graphic_region_from_handle(*buffer2->native_buffer_handle()));

    geom::Rectangle screen_pos1{{0,0} , {512, 512}};
    geom::Rectangle screen_pos2{{80,80} , {592,592}};
    std::list<std::shared_ptr<mg::Renderable>> renderlist
    {
        std::make_shared<DemoRenderable>(buffer2, screen_pos2),
        std::make_shared<DemoRenderable>(buffer1, screen_pos1)
    };

    while (running)
    {
        display->for_each_display_buffer([&](mg::DisplayBuffer& buffer)
        {
            buffer.make_current();
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
