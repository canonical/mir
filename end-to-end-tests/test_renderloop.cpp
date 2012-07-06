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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/graphic_buffer_allocator_factory.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"
#include "mir/display_server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
struct MockDisplay : mg::Display
{
public:
    MOCK_METHOD0(view_area, geom::Rectangle ());
    MOCK_METHOD0(notify_update, void());
};

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    std::shared_ptr<mc::Buffer> alloc_buffer(
        geom::Width /*width*/,
        geom::Height /*height*/,
        mc::PixelFormat /*pf*/)
    {
        return std::shared_ptr<mc::Buffer>();
    }
};


class DisplayServerFixture : public ::testing::Test
{
public:
    DisplayServerFixture()
            : allocation_strategy(std::make_shared<StubGraphicBufferAllocator>()),
              display_server(&allocation_strategy)
    {
    }

    mc::DoubleBufferAllocationStrategy allocation_strategy;
    mir::DisplayServer display_server;
};

}

TEST_F(DisplayServerFixture, notify_sync_and_see_paint)
{
    using namespace testing;

    MockDisplay display;
    EXPECT_CALL(display, notify_update()).Times(1);

    EXPECT_CALL(display, view_area()).Times(AtLeast(1))
            .WillRepeatedly(Return(geom::Rectangle()));

    display_server.render(&display);
}
