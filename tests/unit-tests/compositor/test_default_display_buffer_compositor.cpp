/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/compositor/default_display_buffer_compositor.h"
#include "src/server/report/null_report_factory.h"
#include "mir/compositor/scene.h"
#include "mir/renderer/renderer.h"
#include "mir/geometry/rectangle.h"
#include "mir/test/doubles/mock_renderer.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_display_sink.h"
#include "mir/test/doubles/fake_renderable.h"
#include "mir/test/doubles/mock_compositor_report.h"
#include "mir/test/doubles/stub_scene_element.h"
#include "mir/test/doubles/stub_gl_rendering_provider.h"
#include "mir/test/doubles/stub_output_filter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace ms = mir::scene;
namespace mr = mir::report;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

glm::mat2 const no_transformation(1);

struct StubSceneElement : mc::SceneElement
{
    StubSceneElement(std::shared_ptr<mg::Renderable> const& renderable) :
        renderable_{renderable}
    {
    }

    std::shared_ptr<mir::graphics::Renderable> renderable() const override
    {
        return renderable_;
    }

    void rendered() override
    {
    }

    void occluded() override
    {
    }

private:
    std::shared_ptr<mg::Renderable> const renderable_;
};

auto make_scene_elements(std::initializer_list<std::shared_ptr<mg::Renderable>> list)
    -> mc::SceneElementSequence
{
    mc::SceneElementSequence elements;
    for(auto& entry : list)
        elements.push_back(std::make_shared<StubSceneElement>(entry));
    return elements;
}

struct DefaultDisplayBufferCompositor : public testing::Test
{
    DefaultDisplayBufferCompositor()
     : small(std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{10, 20},{30, 40}})),
       big(std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{5, 10},{100, 200}})),
       fullscreen(std::make_shared<mtd::FakeRenderable>(screen))
    {
        using namespace testing;
        ON_CALL(display_sink, transformation())
            .WillByDefault(Return(no_transformation));
        ON_CALL(display_sink, view_area())
            .WillByDefault(Return(screen));
        ON_CALL(display_sink, overlay(_))
            .WillByDefault(Return(false));
    }

    testing::NiceMock<mtd::MockRenderer> mock_renderer;
    geom::Rectangle screen{{0, 0}, {1366, 768}};
    testing::NiceMock<mtd::MockDisplaySink> display_sink;
    mtd::StubGlRenderingProvider gl_provider;
    std::shared_ptr<mtd::FakeRenderable> small;
    std::shared_ptr<mtd::FakeRenderable> big;
    std::shared_ptr<mtd::FakeRenderable> fullscreen;
};
}

TEST_F(DefaultDisplayBufferCompositor, composite_returns_false_when_scene_elements_are_empty_and_this_is_first_composite)
{
    using namespace testing;

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());
    EXPECT_FALSE(compositor.composite(make_scene_elements({})));
}

TEST_F(DefaultDisplayBufferCompositor, composite_returns_true_when_scene_elements_is_not_empty_and_this_is_first_composite)
{
    using namespace testing;

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());
    EXPECT_TRUE(compositor.composite(make_scene_elements({big})));
}

TEST_F(DefaultDisplayBufferCompositor, renderer_is_suspended_when_we_have_composited_twice_in_a_row_with_the_same_elements)
{
    using namespace testing;

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());
    compositor.composite(make_scene_elements({big}));
    compositor.composite(make_scene_elements({}));

    EXPECT_CALL(display_sink, overlay(_))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_renderer, suspend())
        .Times(1);
    compositor.composite(make_scene_elements({}));
}

TEST_F(DefaultDisplayBufferCompositor, all_expected_reports_are_received_during_composite)
{
    using namespace testing;
    auto report = std::make_shared<mtd::MockCompositorReport>();

    Sequence seq;
    EXPECT_CALL(*report, began_frame(_))
        .InSequence(seq);
    EXPECT_CALL(*report, renderables_in_frame(_,_))
        .InSequence(seq);
    EXPECT_CALL(*report, rendered_frame(_))
        .InSequence(seq);
    EXPECT_CALL(*report, finished_frame(_))
        .InSequence(seq);

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        report);
    compositor.composite(make_scene_elements({big}));
}

TEST_F(DefaultDisplayBufferCompositor, elements_provided_to_composite_are_rendered_in_order)
{
    using namespace testing;
    EXPECT_CALL(mock_renderer, render(ContainerEq(mg::RenderableList{big, small})))
        .Times(1);

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());

    compositor.composite(make_scene_elements({
        big,
        small
    }));
}

TEST_F(DefaultDisplayBufferCompositor, viewport_is_rotated_when_display_sink_view_area_is_rotated)
{   // Regression test for LP: #1643488
    using namespace testing;

    glm::mat2 const rotate_left( 0, 1,  // transposed
                                -1, 0);

    geom::Rectangle const rotated_screen{
        screen.top_left,
        {screen.size.height.as_int(), screen.size.width.as_int()}};

    ON_CALL(display_sink, view_area())
        .WillByDefault(Return(rotated_screen));

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());

    Sequence render_seq;
    EXPECT_CALL(display_sink, transformation())
        .WillOnce(Return(rotate_left));
    EXPECT_CALL(mock_renderer, set_output_transform(rotate_left))
        .InSequence(render_seq);
    EXPECT_CALL(mock_renderer, set_viewport(rotated_screen))
        .InSequence(render_seq);

    compositor.composite(make_scene_elements({big}));
}

TEST_F(DefaultDisplayBufferCompositor, renderer_is_suspended_when_we_have_composited_many_times_in_a_row_with_the_same_elements)
{
    using namespace testing;
    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());

    compositor.composite(make_scene_elements({big}));
    compositor.composite(make_scene_elements({}));

    EXPECT_CALL(display_sink, overlay(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mock_renderer, suspend())
        .Times(1);

    compositor.composite(make_scene_elements({}));

    EXPECT_CALL(mock_renderer, suspend())
        .Times(1);
    compositor.composite(make_scene_elements({}));

    fullscreen->set_buffer({});  // Avoid GMock complaining about false leaks
}

TEST_F(DefaultDisplayBufferCompositor, occluded_surfaces_are_not_rendered)
{
    using namespace testing;

    auto window0 = std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{99,99},{2,2}});
    auto window1 = std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{10,10},{20,20}});
    auto window2 = std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0,0},{100,100}});
    auto window3 = std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0,0},{100,100}});

    mg::RenderableList const visible{window0, window3};

    EXPECT_CALL(mock_renderer, render(ContainerEq(visible)));

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());
    compositor.composite(make_scene_elements({
        window0, //not occluded
        window1, //occluded
        window2, //occluded
        window3  //not occluded
    }));
}

namespace
{
struct MockSceneElement : mc::SceneElement
{
    MockSceneElement(std::shared_ptr<mg::Renderable> const& renderable)
    {
        ON_CALL(*this, renderable())
            .WillByDefault(testing::Return(renderable));
    }

    MOCK_METHOD(std::shared_ptr<mir::graphics::Renderable>, renderable, (), (const, override));
    MOCK_METHOD(void, rendered, (), (override));
    MOCK_METHOD(void, occluded, (), (override));
};
}

TEST_F(DefaultDisplayBufferCompositor, marks_rendered_scene_elements)
{
    using namespace testing;

    auto element0_rendered = std::make_shared<NiceMock<MockSceneElement>>(
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{99,99},{2,2}}));
    auto element1_rendered = std::make_shared<NiceMock<MockSceneElement>>(
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0,0},{100,100}}));

    EXPECT_CALL(*element0_rendered, rendered());
    EXPECT_CALL(*element1_rendered, rendered());

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());

    compositor.composite({element0_rendered, element1_rendered});
}

TEST_F(DefaultDisplayBufferCompositor, marks_occluded_scene_elements)
{
    using namespace testing;

    auto element0_occluded = std::make_shared<NiceMock<MockSceneElement>>(
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{10,10},{20,20}}));
    auto element1_rendered = std::make_shared<NiceMock<MockSceneElement>>(
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0,0},{100,100}}));
    auto element2_occluded = std::make_shared<NiceMock<MockSceneElement>>(
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{10000,10000},{20,20}}));

    EXPECT_CALL(*element0_occluded, occluded());
    EXPECT_CALL(*element1_rendered, rendered());
    EXPECT_CALL(*element2_occluded, occluded());

    mc::DefaultDisplayBufferCompositor compositor(
        display_sink,
        gl_provider,
        mt::fake_shared(mock_renderer),
        std::make_shared<mtd::StubOutputFilter>(),
        mr::null_compositor_report());

    compositor.composite({element0_occluded, element1_rendered, element2_occluded});
}
