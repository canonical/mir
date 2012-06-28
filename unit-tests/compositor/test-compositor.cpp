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

#include "mir/compositor/compositor.h"
#include "mir/compositor/buffer_manager.h"
#include "mir/surfaces/scenegraph.h"
#include "mir/graphics/display.h"
#include "mir/geometry/rectangle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mg = mir::graphics;

namespace
{

class MockBufferTextureBinder : public mc::BufferTextureBinder
{
public:
    MOCK_METHOD0(bind_buffer_to_texture, void ());
};

struct MockScenegraph : ms::Scenegraph
{
public:
    MOCK_METHOD1(get_surfaces_in, ms::surfaces_to_render (geom::Rectangle const&));
};

struct MockDisplay : mg::Display
{
public:
    MOCK_METHOD0(view_area, geom::Rectangle ());
};
}


TEST(Compositor, render)
{
    using namespace testing;

    MockBufferTextureBinder buffer_texture_binder;
    MockScenegraph scenegraph;
    MockDisplay display;

    mc::Compositor comp(&scenegraph, &buffer_texture_binder);

    EXPECT_CALL(buffer_texture_binder, bind_buffer_to_texture()).Times(AtLeast(1));

    EXPECT_CALL(display, view_area())
			.WillRepeatedly(Return(geom::Rectangle()));

    EXPECT_CALL(scenegraph, get_surfaces_in(_))
    		.WillRepeatedly(Return(ms::surfaces_to_render()));

    comp.render(&display);
}
