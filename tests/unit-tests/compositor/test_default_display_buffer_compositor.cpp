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

#include "mir/compositor/default_display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/overlay_renderer.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/renderer.h"
#include "mir/compositor/renderer_factory.h"
#include "mir/compositor/compositing_criteria.h"
#include "mir/geometry/rectangle.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_compositing_criteria.h"
#include "mir_test_doubles/stub_compositing_criteria.h"
#include "mir_test_doubles/null_display_buffer.h"
#include "mir_test_doubles/mock_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct MockScene : mc::Scene
{
    MOCK_METHOD2(for_each_if, void(mc::FilterForScene&, mc::OperatorForScene&));
    MOCK_METHOD1(set_change_callback, void(std::function<void()> const&));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};

struct MockOverlayRenderer : public mc::OverlayRenderer
{
    MOCK_METHOD2(render, void(geom::Rectangle const&, std::function<void(std::shared_ptr<void> const&)>));

    ~MockOverlayRenderer() noexcept {}
};

struct FakeScene : mc::Scene
{
    FakeScene(std::vector<mc::CompositingCriteria*> surfaces) :
        surfaces(surfaces)
    {
    }

    // Ugly...should we use delegation?
    void for_each_if(mc::FilterForScene& filter, mc::OperatorForScene& renderable_operator)
    {
        for (auto it = surfaces.begin(); it != surfaces.end(); it++)
        {
            mc::CompositingCriteria &info = **it;
            if (filter(info)) renderable_operator(info, stub_stream);
        }
    }

    void set_change_callback(std::function<void()> const&) {}

    void change(const std::vector<mc::CompositingCriteria*> &surfs)
    {
        surfaces = surfs;
    }

    void lock() {}
    void unlock() {}

    mtd::MockBufferStream stub_stream;
    std::vector<mc::CompositingCriteria*> surfaces;
};

struct WrappingRenderer : mc::Renderer
{
    WrappingRenderer(mc::Renderer* renderer)
        : renderer{renderer}
    {
    }

    void clear(unsigned long f) override { renderer->clear(f); }
    void render(std::function<void(std::shared_ptr<void> const&)> save_resource,
                mc::CompositingCriteria const& info, mir::surfaces::BufferStream& stream)
    {
        renderer->render(save_resource, info, stream);
    }

    mc::Renderer* const renderer;
};

struct StubRendererFactory : mc::RendererFactory
{
    std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&)
    {
        return std::unique_ptr<WrappingRenderer>(
            new WrappingRenderer{&mock_renderer});
    }

    testing::NiceMock<mtd::MockSurfaceRenderer> mock_renderer;
};

ACTION_P(InvokeArgWithParam, param)
{
    arg0(param);
}

}

TEST(DefaultDisplayBufferCompositor, render)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    MockScene scene;
    NiceMock<MockOverlayRenderer> overlay_renderer;
    mtd::MockDisplayBuffer display_buffer;

    EXPECT_CALL(renderer_factory.mock_renderer, render(_,_,_)).Times(0);

    EXPECT_CALL(display_buffer, view_area())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(geom::Rectangle()));

    EXPECT_CALL(display_buffer, make_current())
        .Times(1);

    EXPECT_CALL(display_buffer, post_update())
            .Times(1);

    EXPECT_CALL(scene, for_each_if(_,_))
                .Times(1);

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    comp->composite();
}

TEST(DefaultDisplayBufferCompositor, render_overlay)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    NiceMock<MockScene> scene;
    NiceMock<mtd::MockDisplayBuffer> display_buffer;
    MockOverlayRenderer overlay_renderer;

    ON_CALL(display_buffer, view_area())
        .WillByDefault(Return(geom::Rectangle()));

    EXPECT_CALL(overlay_renderer, render(_, _)).Times(1);

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    comp->composite();
}

TEST(DefaultDisplayBufferCompositor, skips_scene_that_should_not_be_rendered)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    mtd::NullDisplayBuffer display_buffer;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    NiceMock<mtd::MockCompositingCriteria> mock_criteria1, mock_criteria2, mock_criteria3;

    EXPECT_CALL(mock_criteria1, should_be_rendered_in(_)).WillOnce(Return(true));
    EXPECT_CALL(mock_criteria2, should_be_rendered_in(_)).WillOnce(Return(false));
    EXPECT_CALL(mock_criteria3, should_be_rendered_in(_)).WillOnce(Return(true));

    std::vector<mc::CompositingCriteria*> renderable_vec;
    renderable_vec.push_back(&mock_criteria1);
    renderable_vec.push_back(&mock_criteria2);
    renderable_vec.push_back(&mock_criteria3);

    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(mock_criteria1),_)).Times(1);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(mock_criteria2),_)).Times(0);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(mock_criteria3),_)).Times(1);

    FakeScene scene(renderable_vec);

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    comp->composite();
}

TEST(DefaultDisplayBufferCompositor, bypass_skips_composition)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    geom::Rectangle screen{{0, 0}, {1366, 768}};

    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(0);
    EXPECT_CALL(display_buffer, post_update())
        .Times(0);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(true));

    mtd::StubCompositingCriteria small(10, 20, 30, 40);
    mtd::StubCompositingCriteria fullscreen(0, 0, 1366, 768);

    std::vector<mc::CompositingCriteria*> renderable_vec;
    renderable_vec.push_back(&small);
    renderable_vec.push_back(&fullscreen);

    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(small),_))
        .Times(0);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(fullscreen),_))
        .Times(0);

    FakeScene scene(renderable_vec);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .WillOnce(Return(true));
    EXPECT_CALL(scene.stub_stream, lock_compositor_buffer(_))
        .WillOnce(Return(compositor_buffer));

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    comp->composite();
}

TEST(DefaultDisplayBufferCompositor, obscured_fullscreen_does_not_bypass)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    geom::Rectangle screen{{0, 0}, {1366, 768}};

    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(true));

    mtd::StubCompositingCriteria fullscreen(0, 0, 1366, 768);
    mtd::StubCompositingCriteria small(10, 20, 30, 40);

    std::vector<mc::CompositingCriteria*> renderable_vec;
    renderable_vec.push_back(&fullscreen);
    renderable_vec.push_back(&small);

    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(small),_))
        .Times(1);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(fullscreen),_))
        .Times(1);

    FakeScene scene(renderable_vec);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);
    EXPECT_CALL(scene.stub_stream, lock_compositor_buffer(_))
        .Times(0);

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    comp->composite();
}

TEST(DefaultDisplayBufferCompositor, platform_does_not_support_bypass)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    geom::Rectangle screen{{0, 0}, {1366, 768}};

    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(false));

    mtd::StubCompositingCriteria small(10, 20, 30, 40);
    mtd::StubCompositingCriteria fullscreen(0, 0, 1366, 768);

    std::vector<mc::CompositingCriteria*> renderable_vec;
    renderable_vec.push_back(&small);
    renderable_vec.push_back(&fullscreen);

    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(small),_))
        .Times(1);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(fullscreen),_))
        .Times(1);

    FakeScene scene(renderable_vec);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);
    EXPECT_CALL(scene.stub_stream, lock_compositor_buffer(_))
        .Times(0);

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    comp->composite();
}

TEST(DefaultDisplayBufferCompositor, bypass_aborted_for_incompatible_buffers)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    geom::Rectangle screen{{0, 0}, {1366, 768}};

    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(true));

    mtd::StubCompositingCriteria small(10, 20, 30, 40);
    mtd::StubCompositingCriteria fullscreen(0, 0, 1366, 768);

    std::vector<mc::CompositingCriteria*> renderable_vec;
    renderable_vec.push_back(&small);
    renderable_vec.push_back(&fullscreen);

    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(small),_))
        .Times(1);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(fullscreen),_))
        .Times(1);

    FakeScene scene(renderable_vec);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(scene.stub_stream, lock_compositor_buffer(_))
        .WillOnce(Return(compositor_buffer));
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .WillRepeatedly(Return(false));

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    comp->composite();
}

TEST(DefaultDisplayBufferCompositor, bypass_toggles_seamlessly)
{
    using namespace testing;

    StubRendererFactory renderer_factory;
    NiceMock<MockOverlayRenderer> overlay_renderer;

    geom::Rectangle screen{{0, 0}, {1366, 768}};

    mtd::MockDisplayBuffer display_buffer;
    EXPECT_CALL(display_buffer, view_area())
        .WillRepeatedly(Return(screen));
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(display_buffer, can_bypass())
        .WillRepeatedly(Return(true));

    mtd::StubCompositingCriteria fullscreen(0, 0, 1366, 768);
    mtd::StubCompositingCriteria small(10, 20, 30, 40);

    std::vector<mc::CompositingCriteria*> renderable_vec;
    renderable_vec.push_back(&fullscreen);
    renderable_vec.push_back(&small);

    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(small),_))
        .Times(1);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(fullscreen),_))
        .Times(1);

    FakeScene scene(renderable_vec);

    auto compositor_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);
    EXPECT_CALL(scene.stub_stream, lock_compositor_buffer(_))
        .Times(0);

    mc::DefaultDisplayBufferCompositorFactory factory(
        mt::fake_shared(scene),
        mt::fake_shared(renderer_factory),
        mt::fake_shared(overlay_renderer));

    auto comp = factory.create_compositor_for(display_buffer);

    // Frame 1: small window over fullscreen = no bypass
    comp->composite();

    // Frame 2: fullscreen over small window = bypass
    renderable_vec.resize(0);
    renderable_vec.push_back(&small);
    renderable_vec.push_back(&fullscreen);
    scene.change(renderable_vec);
    EXPECT_CALL(display_buffer, make_current())
        .Times(0);
    EXPECT_CALL(display_buffer, post_update())
        .Times(0);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(small),_))
        .Times(0);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(fullscreen),_))
        .Times(0);
    EXPECT_CALL(scene.stub_stream, lock_compositor_buffer(_))
        .WillOnce(Return(compositor_buffer));
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .WillOnce(Return(true));
    comp->composite();

    // Frame 3: only a small window = no bypass
    renderable_vec.resize(0);
    renderable_vec.push_back(&small);
    scene.change(renderable_vec);
    EXPECT_CALL(display_buffer, make_current())
        .Times(1);
    EXPECT_CALL(display_buffer, post_update())
        .Times(1);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(small),_))
        .Times(1);
    EXPECT_CALL(renderer_factory.mock_renderer, render(_,Ref(fullscreen),_))
        .Times(0);
    EXPECT_CALL(scene.stub_stream, lock_compositor_buffer(_))
        .Times(0);
    EXPECT_CALL(*compositor_buffer, can_bypass())
        .Times(0);
    comp->composite();
}

