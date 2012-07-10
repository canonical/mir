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
#include "mir/surfaces/scenegraph.h"
#include "mir/surfaces/surface_renderer.h"
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

struct MockSurfaceRenderer : public ms::SurfaceRenderer
{
    MOCK_METHOD1(render, void(const std::shared_ptr<ms::Surface>&));
};

struct MockScenegraph : ms::Scenegraph
{
public:
    MOCK_METHOD1(get_surfaces_in, std::shared_ptr<ms::Scenegraph::View> (geom::Rectangle const&));
};

struct MockDisplay : mg::Display
{
public:
    MOCK_METHOD0(view_area, geom::Rectangle ());
    MOCK_METHOD0(notify_update, void ());
};

struct MockView : public ms::Scenegraph::View
{
    MOCK_METHOD1(apply, void(ms::SurfaceRenderer*));
};

struct EmptyDeleter
{
    template<typename T>
    void operator()(T*) const
    {
    }
};

}


TEST(Compositor, render)
{
    using namespace testing;

    MockSurfaceRenderer mock_renderer;
    std::shared_ptr<ms::SurfaceRenderer> renderer(
        &mock_renderer,
        EmptyDeleter());
    MockScenegraph scenegraph;
    MockDisplay display;
    MockView view;
    
    mc::Compositor comp(&scenegraph, renderer);

    EXPECT_CALL(view, apply(_)).Times(1);
    
    EXPECT_CALL(mock_renderer, render(_)).Times(0);
    
    EXPECT_CALL(display, view_area())
            .Times(1)
            .WillRepeatedly(Return(geom::Rectangle()));

    EXPECT_CALL(scenegraph, get_surfaces_in(_))
            .Times(1)
            .WillRepeatedly(
                Return(
                    std::shared_ptr<MockView>(&view, EmptyDeleter())));
    
    EXPECT_CALL(display, notify_update())
            .Times(1);
    
    comp.render(&display);
}
