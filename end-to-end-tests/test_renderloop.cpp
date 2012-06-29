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

#include "mir/graphics/display.h"
#include "mir/graphics/framebuffer_backend.h"
#include "mir/compositor/drawer.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/buffer_manager.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/surfaces/scenegraph.h"
#include "mir/geometry/rectangle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{
struct MockAllocator : mc::GraphicBufferAllocator
{
public:
    MOCK_METHOD3(alloc_buffer, std::shared_ptr<mc::Buffer>(uint32_t, uint32_t, mc::PixelFormat));
    MOCK_METHOD1(free_buffer, void(std::shared_ptr<mc::Buffer>));
};

struct MockScenegraph : ms::Scenegraph
{
public:
    MOCK_METHOD1(get_surfaces_in, ms::SurfacesToRender (geom::Rectangle const&));
};

struct MockDisplay : mg::Display
{
public:
    MOCK_METHOD0(view_area, geom::Rectangle ());
    MOCK_METHOD1(notify_update, void (mg::Texture const&));
};

}

TEST(compositor_renderloop, notify_sync_and_see_paint)
{
    using namespace testing;

    MockAllocator gr_allocator;
    MockScenegraph scenegraph;
    MockDisplay display;

    mc::BufferManager buffer_manager(&gr_allocator);
    mc::Drawer&& comp = mc::Compositor(&scenegraph, &buffer_manager);

    EXPECT_CALL(display, notify_update(_)).Times(1);

    EXPECT_CALL(display, view_area()).Times(AtLeast(1))
			.WillRepeatedly(Return(geom::Rectangle()));

    EXPECT_CALL(scenegraph, get_surfaces_in(_)).Times(AtLeast(1))
    		.WillRepeatedly(Return(ms::SurfacesToRender()));

    comp.render(&display);
}
