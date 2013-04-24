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

#include "mir/compositor/default_compositing_strategy.h"
#include "mir/compositor/overlay_renderer.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/renderer.h"
#include "mir/geometry/rectangle.h"
#include "mir_test_doubles/mock_renderable.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/null_display_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct MockRenderables : mc::Renderables
{
    MOCK_METHOD2(for_each_if, void(mc::FilterForRenderables&, mc::OperatorForRenderables&));
    MOCK_METHOD1(set_change_callback, void(std::function<void()> const&));
};

struct MockOverlayRenderer : public mc::OverlayRenderer
{
    MOCK_METHOD1(render, void(mg::DisplayBuffer&));
};

struct FakeRenderables : mc::Renderables
{
    FakeRenderables(std::vector<mg::Renderable*> renderables) :
        renderables(renderables)
    {
    }

    // Ugly...should we use delegation?
    void for_each_if(mc::FilterForRenderables& filter, mc::OperatorForRenderables& renderable_operator)
    {
        for (auto it = renderables.begin(); it != renderables.end(); it++)
        {
            mg::Renderable &renderable = **it;
            if (filter(renderable)) renderable_operator(renderable);
        }
    }

    void set_change_callback(std::function<void()> const&) {}

    std::vector<mg::Renderable*> renderables;
};

ACTION_P(InvokeArgWithParam, param)
{
    arg0(param);
}

}

TEST(DefaultCompositingStrategy, render)
{
    using namespace testing;

    mtd::MockSurfaceRenderer mock_renderer;
    MockRenderables renderables;
    NiceMock<MockOverlayRenderer> overlay_renderer;
    mtd::MockDisplayBuffer display_buffer;

    mc::DefaultCompositingStrategy comp(mt::fake_shared(renderables),
                                        mt::fake_shared(mock_renderer),
                                        mt::fake_shared(overlay_renderer));

    EXPECT_CALL(mock_renderer, render(_,_)).Times(0);

    EXPECT_CALL(display_buffer, view_area())
        .Times(1)
        .WillOnce(Return(geom::Rectangle()));

    EXPECT_CALL(display_buffer, make_current())
        .Times(1);

    EXPECT_CALL(display_buffer, clear())
        .Times(1);

    EXPECT_CALL(display_buffer, post_update())
            .Times(1);

    EXPECT_CALL(renderables, for_each_if(_,_))
                .Times(1);

    comp.render(display_buffer);
}

TEST(DefaultCompositingStrategy, render_overlay)
{
    using namespace testing;

    NiceMock<mtd::MockSurfaceRenderer> mock_renderer;
    NiceMock<MockRenderables> renderables;
    NiceMock<mtd::MockDisplayBuffer> display_buffer;
    MockOverlayRenderer overlay_renderer;

    mc::DefaultCompositingStrategy comp(mt::fake_shared(renderables),
                                        mt::fake_shared(mock_renderer),
                                        mt::fake_shared(overlay_renderer));
    ON_CALL(display_buffer, view_area())
        .WillByDefault(Return(geom::Rectangle()));
    
    EXPECT_CALL(overlay_renderer, render(_)).Times(1);

    comp.render(display_buffer);
}

TEST(DefaultCompositingStrategy, skips_invisible_renderables)
{
    using namespace testing;

    mtd::MockSurfaceRenderer mock_renderer;
    mtd::NullDisplayBuffer display_buffer;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    NiceMock<mtd::MockRenderable> mr1, mr2, mr3;

    EXPECT_CALL(mr1, hidden()).WillOnce(Return(false));
    EXPECT_CALL(mr2, hidden()).WillOnce(Return(true));
    EXPECT_CALL(mr3, hidden()).WillOnce(Return(false));

    std::vector<mg::Renderable*> renderable_vec;
    renderable_vec.push_back(&mr1);
    renderable_vec.push_back(&mr2);
    renderable_vec.push_back(&mr3);

    EXPECT_CALL(mock_renderer, render(_,Ref(mr1))).Times(1);
    EXPECT_CALL(mock_renderer, render(_,Ref(mr2))).Times(0);
    EXPECT_CALL(mock_renderer, render(_,Ref(mr3))).Times(1);

    FakeRenderables renderables(renderable_vec);

    mc::DefaultCompositingStrategy comp(mt::fake_shared(renderables),
                                        mt::fake_shared(mock_renderer),
                                        mt::fake_shared(overlay_renderer));

    comp.render(display_buffer);
}
