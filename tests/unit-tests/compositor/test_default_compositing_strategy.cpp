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
#include "mir/compositor/renderables.h"
#include "mir/graphics/renderer.h"
#include "mir/graphics/compositing_criteria.h"
#include "mir/geometry/rectangle.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_compositing_criteria.h"
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
    MOCK_METHOD2(render, void(geom::Rectangle const&, std::function<void(std::shared_ptr<void> const&)>));

    ~MockOverlayRenderer() noexcept {}
};

struct FakeRenderables : mc::Renderables
{
    FakeRenderables(std::vector<mg::CompositingCriteria*> surfaces) :
        surfaces(surfaces)
    {
    }

    // Ugly...should we use delegation?
    void for_each_if(mc::FilterForRenderables& filter, mc::OperatorForRenderables& renderable_operator)
    {
        for (auto it = surfaces.begin(); it != surfaces.end(); it++)
        {
            mg::CompositingCriteria &info = **it;
            if (filter(info)) renderable_operator(info, stub_stream);
        }
    }

    void set_change_callback(std::function<void()> const&) {}

    mtd::MockBufferStream stub_stream;
    std::vector<mg::CompositingCriteria*> surfaces;
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

    EXPECT_CALL(mock_renderer, render(_,_,_)).Times(0);

    EXPECT_CALL(display_buffer, view_area())
        .Times(1)
        .WillOnce(Return(geom::Rectangle()));

    EXPECT_CALL(display_buffer, make_current())
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
    
    EXPECT_CALL(overlay_renderer, render(_, _)).Times(1);

    comp.render(display_buffer);
}

TEST(DefaultCompositingStrategy, skips_renderables_that_should_not_be_rendered)
{
    using namespace testing;

    mtd::MockSurfaceRenderer mock_renderer;
    mtd::NullDisplayBuffer display_buffer;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    NiceMock<mtd::MockCompositingCriteria> mock_criteria1, mock_criteria2, mock_criteria3;

    EXPECT_CALL(mock_criteria1, should_be_rendered()).WillOnce(Return(true));
    EXPECT_CALL(mock_criteria2, should_be_rendered()).WillOnce(Return(false));
    EXPECT_CALL(mock_criteria3, should_be_rendered()).WillOnce(Return(true));

    std::vector<mg::CompositingCriteria*> renderable_vec;
    renderable_vec.push_back(&mock_criteria1);
    renderable_vec.push_back(&mock_criteria2);
    renderable_vec.push_back(&mock_criteria3);

    EXPECT_CALL(mock_renderer, render(_,Ref(mock_criteria1),_)).Times(1);
    EXPECT_CALL(mock_renderer, render(_,Ref(mock_criteria2),_)).Times(0);
    EXPECT_CALL(mock_renderer, render(_,Ref(mock_criteria3),_)).Times(1);

    FakeRenderables renderables(renderable_vec);

    mc::DefaultCompositingStrategy comp(mt::fake_shared(renderables),
                                        mt::fake_shared(mock_renderer),
                                        mt::fake_shared(overlay_renderer));

    comp.render(display_buffer);
}
