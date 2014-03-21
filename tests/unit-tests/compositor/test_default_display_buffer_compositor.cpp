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

#include "src/server/compositor/default_display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "src/server/report/null_report_factory.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/renderer.h"
#include "mir/geometry/rectangle.h"
#include "mir_test_doubles/mock_renderer.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/mock_renderable.h"
#include "mir_test_doubles/fake_renderable.h"
#include "mir_test_doubles/null_display_buffer.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_compositor_report.h"
#include "mir_test_doubles/mock_scene.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;

namespace
{

struct FakeScene : mc::Scene
{
    FakeScene(std::vector<mg::Renderable*> surfaces) :
        surfaces(surfaces)
    {
    }

    // Ugly...should we use delegation?
    void for_each_if(mc::FilterForScene& filter, mc::OperatorForScene& renderable_operator)
    {
        for (auto it = surfaces.begin(); it != surfaces.end(); it++)
        {
            mg::Renderable &info = **it;
            if (filter(info)) renderable_operator(info);
        }
    }

    void reverse_for_each_if(mc::FilterForScene &filter,
                             mc::OperatorForScene &op)
    {
        for (auto it = surfaces.rbegin(); it != surfaces.rend(); ++it)
        {
            mg::Renderable &renderable = **it;
            if (filter(renderable))
                op(renderable);
        }
    }

    void set_change_callback(std::function<void()> const&) {}

    void change(const std::vector<mg::Renderable*> &surfs)
    {
        surfaces = surfs;
    }

    void lock() {}
    void unlock() {}

    std::vector<mg::Renderable*> surfaces;
};

struct DefaultDisplayBufferCompositor : public testing::Test
{
    DefaultDisplayBufferCompositor()
    {
        using namespace testing;
        ON_CALL(display_buffer, orientation())
            .WillByDefault(Return(mir_orientation_normal));
        ON_CALL(display_buffer, view_area())
            .WillByDefault(Return(screen));
        ON_CALL(display_buffer, can_bypass())
            .WillByDefault(Return(false));
    }

    testing::NiceMock<mtd::MockRenderer> mock_renderer;
    geom::Rectangle screen{{0, 0}, {1366, 768}};
    mtd::FakeRenderable small{10, 20, 30, 40};
    mtd::FakeRenderable big{5, 10, 100, 200};
    mtd::FakeRenderable fullscreen{0, 0, 1366, 768};
    testing::NiceMock<mtd::MockDisplayBuffer> display_buffer;
};
}

TEST_F(DefaultDisplayBufferCompositor, render)
{
    using namespace testing;
    mtd::MockScene scene;

    EXPECT_CALL(mock_renderer, render(_,_))
        .Times(0);
    EXPECT_CALL(display_buffer, view_area())
        .Times(AtLeast(1));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(scene, for_each_if(_,_))
        .Times(1);
    EXPECT_CALL(scene, reverse_for_each_if(_,_))
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        mr::null_compositor_report());
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, skips_scene_that_should_not_be_rendered)
{
    using namespace testing;
    mtd::NullDisplayBuffer display_buffer;
    mtd::MockRenderable mock_renderable1, mock_renderable2, mock_renderable3;

    auto buf = std::make_shared<mtd::StubBuffer>();
    EXPECT_CALL(mock_renderable1, buffer(_))
        .WillOnce(Return(buf));
    EXPECT_CALL(mock_renderable2, buffer(_))
        .Times(0);
    EXPECT_CALL(mock_renderable3, buffer(_))
        .WillOnce(Return(buf));

    glm::mat4 simple;
    EXPECT_CALL(mock_renderable1, transformation())
        .WillOnce(Return(simple));
    EXPECT_CALL(mock_renderable2, transformation())
        .WillOnce(Return(simple));
    EXPECT_CALL(mock_renderable3, transformation())
        .WillOnce(Return(simple));

    EXPECT_CALL(mock_renderable1, should_be_rendered_in(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mock_renderable2, should_be_rendered_in(_))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mock_renderable3, should_be_rendered_in(_))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(mock_renderable1, alpha())
        .WillOnce(Return(1.0f));
    EXPECT_CALL(mock_renderable3, alpha())
        .WillOnce(Return(1.0f));

    EXPECT_CALL(mock_renderable1, shaped())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_renderable3, shaped())
        .WillOnce(Return(false));

    EXPECT_CALL(mock_renderable1, screen_position())
        .WillOnce(Return(geom::Rectangle{{1,2}, {3,4}}));
    EXPECT_CALL(mock_renderable2, screen_position())
        .Times(0);
    EXPECT_CALL(mock_renderable3, screen_position())
        .WillOnce(Return(geom::Rectangle{{5,6}, {7,8}}));

    FakeScene scene({
        &mock_renderable1,
        &mock_renderable2,
        &mock_renderable3
    });

    EXPECT_CALL(mock_renderer, render(Ref(mock_renderable1),_))
        .Times(1);
    EXPECT_CALL(mock_renderer, render(Ref(mock_renderable2),_))
        .Times(0);
    EXPECT_CALL(mock_renderer, render(Ref(mock_renderable3),_))
        .Times(1);

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        mr::null_compositor_report());
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, bypass_skips_composition)
{
    using namespace testing;
    ON_CALL(display_buffer, can_bypass())
        .WillByDefault(Return(true));

    FakeScene scene({
        &small,
        &fullscreen
    });

    EXPECT_CALL(mock_renderer, suspend())
        .Times(1);
    EXPECT_CALL(mock_renderer, begin())
        .Times(0);
    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(0);
    EXPECT_CALL(mock_renderer, render(Ref(fullscreen),_))
        .Times(0);
    EXPECT_CALL(mock_renderer, end())
        .Times(0);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    fullscreen.set_buffer(compositor_buffer);
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .WillOnce(Return(true));

    auto report = std::make_shared<mtd::MockCompositorReport>();
    EXPECT_CALL(*report, began_frame(_));
    EXPECT_CALL(*report, finished_frame(true,_));

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        report);
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, calls_renderer_in_sequence)
{
    using namespace testing;
    FakeScene scene({
        &big,
        &small
    });

    Sequence render_seq;
    EXPECT_CALL(mock_renderer, suspend())
        .Times(0);
    EXPECT_CALL(display_buffer, make_current())
        .InSequence(render_seq);
    EXPECT_CALL(display_buffer, orientation())
        .InSequence(render_seq)
        .WillOnce(Return(mir_orientation_normal));
    EXPECT_CALL(mock_renderer, set_rotation(_))
        .InSequence(render_seq);
    EXPECT_CALL(mock_renderer, begin())
        .InSequence(render_seq);
    EXPECT_CALL(mock_renderer, render(Ref(big),_))
        .InSequence(render_seq);
    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .InSequence(render_seq);
    EXPECT_CALL(mock_renderer, end())
        .InSequence(render_seq);
    EXPECT_CALL(display_buffer, post_update())
        .InSequence(render_seq);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        mr::null_compositor_report());
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, obscured_fullscreen_does_not_bypass)
{
    using namespace testing;
    ON_CALL(display_buffer, can_bypass())
        .WillByDefault(Return(true));

    FakeScene scene({
        &fullscreen,
        &small
    });

    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(1);
    EXPECT_CALL(mock_renderer, render(Ref(fullscreen),_))
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);

    auto report = std::make_shared<mtd::MockCompositorReport>();
    EXPECT_CALL(*report, began_frame(_));
    EXPECT_CALL(*report, finished_frame(false,_));

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        report);
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, platform_does_not_support_bypass)
{
    using namespace testing;
    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, orientation())
        .WillOnce(Return(mir_orientation_normal));
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(false));

    FakeScene scene({
        &small,
        &fullscreen
    });

    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(0);  // zero due to occlusion detection
    EXPECT_CALL(mock_renderer, render(Ref(fullscreen),_))
        .Times(1);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        mr::null_compositor_report());
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, bypass_aborted_for_incompatible_buffers)
{
    using namespace testing;

    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, orientation())
        .WillOnce(Return(mir_orientation_normal));
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(true));

    FakeScene scene({
        &small,
        &fullscreen
    });

    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(0);  // zero due to occlusion detection
    EXPECT_CALL(mock_renderer, render(Ref(fullscreen),_))
        .Times(1);

    auto nonbypassable = std::make_shared<mtd::MockBuffer>();
    fullscreen.set_buffer(nonbypassable);
    EXPECT_CALL(*nonbypassable, can_bypass())
        .WillRepeatedly(Return(false));

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        mr::null_compositor_report());
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, bypass_toggles_seamlessly)
{
    using namespace testing;
    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, orientation())
        .WillRepeatedly(Return(mir_orientation_normal));
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(true));

    FakeScene scene({
        &fullscreen,
        &small
    });

    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(1);
    EXPECT_CALL(mock_renderer, render(Ref(fullscreen),_))
        .Times(1);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);
    fullscreen.set_buffer(compositor_buffer);

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        mr::null_compositor_report());

    // Frame 1: small window over fullscreen = no bypass
    compositor.composite();

    // Frame 2: fullscreen over small window = bypass
    scene.change({
        &small,
        &fullscreen
    });
    EXPECT_CALL(display_buffer, make_current())
        .Times(0);
    EXPECT_CALL(display_buffer, post_update())
        .Times(0);
    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(0);
    EXPECT_CALL(mock_renderer, render(Ref(fullscreen),_))
        .Times(0);
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .WillOnce(Return(true));
    compositor.composite();

    // Frame 3: only a small window = no bypass
    scene.change({&small});
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(1);
    EXPECT_CALL(mock_renderer, render(Ref(fullscreen),_))
        .Times(0);
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);
    compositor.composite();
}

TEST_F(DefaultDisplayBufferCompositor, occluded_surface_is_never_rendered)
{
    using namespace testing;
    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, orientation())
        .WillOnce(Return(mir_orientation_normal));
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(false));

    mtd::FakeRenderable large(0, 0, 100, 100);
    FakeScene scene({
        &small,
        &large
    });

    EXPECT_CALL(mock_renderer, render(Ref(small),_))
        .Times(0);
    EXPECT_CALL(mock_renderer, render(Ref(large),_))
        .Times(1);

    mc::DefaultDisplayBufferCompositor compositor(
        display_buffer,
        mt::fake_shared(scene),
        mt::fake_shared(mock_renderer),
        mr::null_compositor_report());
    compositor.composite();
}
