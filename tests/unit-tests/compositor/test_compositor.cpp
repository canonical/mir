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
#include "mir/graphics/renderable.h"
#include "mir/graphics/renderer.h"
#include "mir/graphics/display.h"
#include "mir/geometry/rectangle.h"
#include "mir_test/mock_display.h"


#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mg = mir::graphics;

namespace
{

struct MockSurfaceRenderer : public mg::Renderer
{
    MOCK_METHOD1(render, void(mg::Renderable&));
};

struct MockScenegraph : ms::Scenegraph
{
public:
    MOCK_METHOD1(get_surfaces_in, std::shared_ptr<ms::SurfaceCollection> (geom::Rectangle const&));
};


struct MockSurfaceCollection : public ms::SurfaceCollection
{
    MOCK_METHOD1(invoke_for_each_surface, void(ms::SurfaceEnumerator&));
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
    std::shared_ptr<mg::Renderer> renderer(
        &mock_renderer,
        EmptyDeleter());
    MockScenegraph scenegraph;
    mg::MockDisplay display;
    MockSurfaceCollection view;

    mc::Compositor comp(&scenegraph, renderer);

    EXPECT_CALL(view, invoke_for_each_surface(_)).Times(1);

    EXPECT_CALL(mock_renderer, render(_)).Times(0);

    EXPECT_CALL(display, view_area())
            .Times(1)
            .WillRepeatedly(Return(geom::Rectangle()));

    EXPECT_CALL(scenegraph, get_surfaces_in(_))
            .Times(1)
            .WillRepeatedly(
                Return(
                    std::shared_ptr<MockSurfaceCollection>(&view, EmptyDeleter())));

    EXPECT_CALL(display, post_update())
            .Times(1);

    comp.render(&display);
}
