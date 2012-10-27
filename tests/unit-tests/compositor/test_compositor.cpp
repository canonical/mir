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
#include "mir_test/empty_deleter.h"

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

struct MockRenderview : mg::Renderview
{
    MOCK_METHOD1(get_renderables_in, std::shared_ptr<mg::RenderableCollection> (geom::Rectangle const&));
};


struct MockRenderableCollection : public mg::RenderableCollection
{
    MOCK_METHOD1(invoke_for_each_renderable, void(mg::RenderableEnumerator&));
};

}


TEST(Compositor, render)
{
    using namespace testing;

    MockSurfaceRenderer mock_renderer;
    std::shared_ptr<mg::Renderer> renderer(
        &mock_renderer,
        mir::EmptyDeleter());
    MockRenderview render_view;
    mg::MockDisplay display;
    MockRenderableCollection view;

    mc::Compositor comp(&render_view, renderer);

    EXPECT_CALL(view, invoke_for_each_renderable(_)).Times(1);

    EXPECT_CALL(mock_renderer, render(_)).Times(0);

    EXPECT_CALL(display, view_area())
            .Times(1)
            .WillRepeatedly(Return(geom::Rectangle()));

    EXPECT_CALL(render_view, get_renderables_in(_))
            .Times(1)
            .WillRepeatedly(
                Return(
                    std::shared_ptr<MockRenderableCollection>(&view, mir::EmptyDeleter())));

    EXPECT_CALL(display, post_update())
            .Times(1);

    comp.render(&display);
}
