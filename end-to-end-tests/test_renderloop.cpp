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
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/surfaces/scenegraph.h"
#include "mir/geometry/rectangle.h"
#include "mir/surfaces/surface_stack.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{
struct MockDisplay : mg::Display
{
public:
    MOCK_METHOD0(view_area, geom::Rectangle ());
    MOCK_METHOD0(notify_update, void());
};

struct MockScenegraph : public ms::Scenegraph
{
    MOCK_METHOD1(get_surfaces_in, ms::SurfacesToRender(const geom::Rectangle&));
};

}

TEST(compositor_renderloop, notify_sync_and_see_paint)
{
    using namespace testing;

    MockScenegraph scenegraph;
    MockDisplay display;

    mc::Drawer&& comp = mc::Compositor(&scenegraph);

    EXPECT_CALL(display, notify_update()).Times(1);    
    EXPECT_CALL(display, view_area()).Times(AtLeast(1))
			.WillRepeatedly(Return(geom::Rectangle()));

    EXPECT_CALL(scenegraph, get_surfaces_in(_)).WillRepeatedly(Return(ms::SurfacesToRender()));
    
    comp.render(&display);
}
