/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "src/server/scene/surface_stack.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/geometry/rectangle.h"
#include "mir/scene/observer.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/compositor/scene_element.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/scene/basic_surface.h"
#include "src/server/compositor/stream.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_buffer_stream.h"
#include "mir/test/doubles/stub_buffer_stream_factory.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/mock_buffer_stream.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <future>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;

namespace
{

void post_a_frame(mc::BufferStream& s)
{
    s.submit_buffer(std::make_shared<mtd::StubBuffer>());
}

MATCHER_P(SurfaceWithInputReceptionMode, mode, "")
{
    return arg->reception_mode() == mode;
}

MATCHER_P(SceneElementForStream, stream, "")
{
    return arg->renderable()->id() == stream.get();
}

struct MockCallback
{
    MOCK_METHOD0(call, void());
};

struct MockSceneObserver : public ms::Observer
{
    MOCK_METHOD1(surface_added, void(std::shared_ptr<ms::Surface> const&));
    MOCK_METHOD1(surface_removed, void(std::shared_ptr<ms::Surface> const&));
    MOCK_METHOD0(surfaces_reordered, void());
    MOCK_METHOD0(scene_changed, void());

    MOCK_METHOD1(surface_exists, void(std::shared_ptr<ms::Surface> const&));
    MOCK_METHOD0(end_observation, void());
};

struct SurfaceStack : public ::testing::Test
{
    void SetUp()
    {
        using namespace testing;
        default_params = ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}});

        stub_surface1 = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{},{}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { stub_buffer_stream1, {}, {} } },
            std::shared_ptr<mg::CursorImage>(),
            report);

        stub_surface2 = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{},{}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { stub_buffer_stream2, {}, {} } },
            std::shared_ptr<mg::CursorImage>(),
            report);

        
        stub_surface3 = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{},{}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { stub_buffer_stream3, {}, {} } },
            std::shared_ptr<mg::CursorImage>(),
            report);

        
        invisible_stub_surface = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{},{}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { std::make_shared<mtd::StubBufferStream>(), {}, {} } },
            std::shared_ptr<mg::CursorImage>(),
            report);
        invisible_stub_surface->hide();
    }

    ms::SurfaceCreationParameters default_params;
    std::shared_ptr<mc::BufferStream> stub_buffer_stream1 = std::make_shared<mtd::StubBufferStream>();
    std::shared_ptr<mc::BufferStream> stub_buffer_stream2 = std::make_shared<mtd::StubBufferStream>();
    std::shared_ptr<mc::BufferStream> stub_buffer_stream3 = std::make_shared<mtd::StubBufferStream>();

    std::shared_ptr<ms::Surface> stub_surface1;
    std::shared_ptr<ms::Surface> stub_surface2;
    std::shared_ptr<ms::Surface> stub_surface3;
    std::shared_ptr<ms::Surface> invisible_stub_surface;

    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    ms::SurfaceStack stack{report};
    void const* compositor_id{&default_params};
    geom::Rectangle view_area;
};

}

TEST_F(SurfaceStack, owns_surface_from_add_to_remove)
{
    using namespace testing;

    auto const use_count = stub_surface1.use_count();

    stack.add_surface(stub_surface1, default_params.input_mode);

    EXPECT_THAT(stub_surface1.use_count(), Gt(use_count));

    stack.remove_surface(stub_surface1);

    EXPECT_THAT(stub_surface1.use_count(), Eq(use_count));
}

TEST_F(SurfaceStack, stacking_order)
{
    using namespace testing;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream3)));
}

TEST_F(SurfaceStack, stacking_order_with_multiple_buffer_streams)
{
    using namespace testing;
    auto stub_stream0 = std::make_shared<mtd::StubBufferStream>();
    auto stub_stream1 = std::make_shared<mtd::StubBufferStream>();
    auto stub_stream2 = std::make_shared<mtd::StubBufferStream>();
    std::list<ms::StreamInfo> streams = {
        { stub_buffer_stream1, {0,0}, {} },
        { stub_stream0, {2,2}, {} },
        { stub_stream1, {2,3}, {} },
    };
    stub_surface1->set_streams(streams);

    streams = {
        { stub_stream2, {2,4}, {} },
        { stub_buffer_stream3, {0,0}, {} }
    };
    stub_surface3->set_streams(streams);

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_stream0),
            SceneElementForStream(stub_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_stream2),
            SceneElementForStream(stub_buffer_stream3)
        ));
}

TEST_F(SurfaceStack, scene_snapshot_omits_invisible_surfaces)
{
    using namespace testing;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(invisible_stub_surface, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2)));
}

TEST_F(SurfaceStack, scene_counts_pending_accurately)
{
    using namespace testing;
    ms::SurfaceStack stack{report};
    stack.register_compositor(this);

    auto stream = std::make_shared<mc::Stream>(geom::Size{ 1, 1 }, mir_pixel_format_abgr_8888);

    auto surface = std::make_shared<ms::BasicSurface>(
        std::string("stub"),
        geom::Rectangle{{},{}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo> { { stream, {}, {} } },
        std::shared_ptr<mg::CursorImage>(),
        report);
    stack.add_surface(surface, default_params.input_mode);
    surface->configure(mir_window_attrib_visibility,
                       mir_window_visibility_exposed);

    EXPECT_EQ(0, stack.frames_pending(this));

    unsigned int num_posts = 3;

    for (auto i = 0u; i != num_posts; i++)
        post_a_frame(*stream);

    for (auto expect = 0u; expect != num_posts; expect++)
    {
        ASSERT_EQ(expect >= num_posts ? 0 : 1, stack.frames_pending(this));
        for (auto& element : stack.scene_elements_for(compositor_id))
            element->renderable()->buffer();
    }
}

TEST_F(SurfaceStack, scene_doesnt_count_pending_frames_from_occluded_surfaces)
{  // Regression test for LP: #1418081
    using namespace testing;

    ms::SurfaceStack stack{report};
    stack.register_compositor(this);
    auto stream = std::make_shared<mtd::StubBufferStream>();
    auto surface = std::make_shared<ms::BasicSurface>(
        std::string("stub"),
        geom::Rectangle{{},{}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo> { { stream, {}, {} } },
        std::shared_ptr<mg::CursorImage>(),
        report);

    stack.add_surface(surface, default_params.input_mode);
    auto elements = stack.scene_elements_for(this);
    for (auto const& elem : elements)
        elem->occluded();

    EXPECT_EQ(0, stack.frames_pending(this));
    post_a_frame(*stream);
    post_a_frame(*stream);
    post_a_frame(*stream);
    EXPECT_EQ(0, stack.frames_pending(this));
}

TEST_F(SurfaceStack, scene_doesnt_count_pending_frames_from_partially_exposed_surfaces)
{  // Regression test for LP: #1499039
    using namespace testing;

    // Partially exposed means occluded in one compositor but not another
    ms::SurfaceStack stack{report};
    auto const comp1 = reinterpret_cast<mc::CompositorID>(0);
    auto const comp2 = reinterpret_cast<mc::CompositorID>(1);

    stack.register_compositor(comp1);
    stack.register_compositor(comp2);
    auto stream = std::make_shared<mtd::StubBufferStream>();
    auto surface = std::make_shared<ms::BasicSurface>(
        std::string("stub"),
        geom::Rectangle{{},{}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo> { { stream, {}, {} } },
        std::shared_ptr<mg::CursorImage>(),
        report);

    stack.add_surface(surface, default_params.input_mode);
    post_a_frame(*stream);
    post_a_frame(*stream);
    post_a_frame(*stream);

    EXPECT_EQ(3, stack.frames_pending(comp1));
    EXPECT_EQ(3, stack.frames_pending(comp2));

    auto elements = stack.scene_elements_for(comp1);
    for (auto const& elem : elements)
    {
        elem->rendered();
    }

    elements = stack.scene_elements_for(comp2);
    for (auto const& elem : elements)
    {
        elem->occluded();
    }

    EXPECT_EQ(3, stack.frames_pending(comp1));
    EXPECT_EQ(0, stack.frames_pending(comp2));
}

TEST_F(SurfaceStack, surfaces_are_emitted_by_layer)
{
    using namespace testing;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream3),
            SceneElementForStream(stub_buffer_stream2)));
}


TEST_F(SurfaceStack, input_registrar_is_notified_of_input_monitor_scene)
{
    using namespace ::testing;

    MockSceneObserver observer;

    stack.add_observer(mt::fake_shared(observer));


    Sequence seq;
    EXPECT_CALL(observer, surface_added(SurfaceWithInputReceptionMode(mi::InputReceptionMode::receives_all_input)))
        .InSequence(seq);
    EXPECT_CALL(observer, surface_removed(_))
        .InSequence(seq);

    stack.add_surface(stub_surface1, mi::InputReceptionMode::receives_all_input);
    stack.remove_surface(stub_surface1);
}

TEST_F(SurfaceStack, raise_to_top_alters_render_ordering)
{
    using namespace ::testing;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream3)));

    stack.raise(stub_surface1);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream3),
            SceneElementForStream(stub_buffer_stream1)));
}

TEST_F(SurfaceStack, raise_throw_behavior)
{
    using namespace ::testing;

    std::shared_ptr<ms::BasicSurface> null_surface{nullptr};
    EXPECT_THROW({
            stack.raise(null_surface);
    }, std::runtime_error);
}

TEST_F(SurfaceStack, generate_elementelements)
{
    using namespace testing;

    size_t num_surfaces{3};

    std::vector<std::shared_ptr<ms::Surface>> surfaces;
    for(auto i = 0u; i < num_surfaces; i++)
    {
        auto const surface = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{geom::Point{3 * i, 4 * i},geom::Size{1 * i, 2 * i}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { std::make_shared<mtd::StubBufferStream>(), {}, {} } },
            std::shared_ptr<mg::CursorImage>(),
            report);
        
        surfaces.emplace_back(surface);
        stack.add_surface(surface, default_params.input_mode);
    }

    auto const elements = stack.scene_elements_for(compositor_id);

    ASSERT_THAT(elements.size(), Eq(num_surfaces));

    auto surface_it = surfaces.begin();
    for(auto& element : elements)
    {
        EXPECT_THAT(element->renderable()->screen_position().top_left, Eq((*surface_it++)->top_left()));
    }

    for(auto& surface : surfaces)
        stack.remove_surface(surface);
}

TEST_F(SurfaceStack, scene_observer_notified_of_add_and_remove)
{
    using namespace ::testing;

    MockSceneObserver observer;

    InSequence seq;
    EXPECT_CALL(observer, surface_added(stub_surface1)).Times(1);
    EXPECT_CALL(observer, surface_removed(stub_surface1))
        .Times(1);

    stack.add_observer(mt::fake_shared(observer));

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.remove_surface(stub_surface1);
}

TEST_F(SurfaceStack, multiple_observers)
{
    using namespace ::testing;

    MockSceneObserver observer1, observer2;

    InSequence seq;
    EXPECT_CALL(observer1, surface_added(stub_surface1)).Times(1);
    EXPECT_CALL(observer2, surface_added(stub_surface1)).Times(1);

    stack.add_observer(mt::fake_shared(observer1));
    stack.add_observer(mt::fake_shared(observer2));

    stack.add_surface(stub_surface1, default_params.input_mode);
}

TEST_F(SurfaceStack, remove_scene_observer)
{
    using namespace ::testing;

    MockSceneObserver observer;

    InSequence seq;
    EXPECT_CALL(observer, surface_added(stub_surface1)).Times(1);
    // We remove the scene observer before removing the surface, and thus
    // expect to NOT see the surface_removed call
    EXPECT_CALL(observer, end_observation()).Times(1);
    EXPECT_CALL(observer, surface_removed(stub_surface1))
        .Times(0);

    stack.add_observer(mt::fake_shared(observer));

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.remove_observer(mt::fake_shared(observer));

    stack.remove_surface(stub_surface1);
}

// Many clients of the scene observer wish to install surface observers to monitor surface
// notifications. We offer them a surface_added event for existing surfaces to give them
// a chance to do this.
TEST_F(SurfaceStack, scene_observer_informed_of_existing_surfaces)
{
    using namespace ::testing;

    MockSceneObserver observer;

    InSequence seq;
    EXPECT_CALL(observer, surface_exists(stub_surface1)).Times(1);
    EXPECT_CALL(observer, surface_exists(stub_surface2)).Times(1);

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    stack.add_observer(mt::fake_shared(observer));
}

TEST_F(SurfaceStack, scene_observer_can_query_scene_within_surface_exists_notification)
{
    using namespace ::testing;

    MockSceneObserver observer;

    auto const scene_query = [&]{
        stack.for_each([&](std::shared_ptr<mi::Surface> const& surface){
            EXPECT_THAT(surface.get(), Eq(stub_surface1.get()));
        });
    };
    EXPECT_CALL(observer, surface_exists(stub_surface1)).Times(1)
        .WillOnce(InvokeWithoutArgs(scene_query));

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_observer(mt::fake_shared(observer));
}

TEST_F(SurfaceStack, scene_observer_can_async_query_scene_within_surface_exists_notification)
{
    using namespace ::testing;

    MockSceneObserver observer;

    auto const scene_query = [&]{
        stack.for_each([&](std::shared_ptr<mi::Surface> const& surface){
            EXPECT_THAT(surface.get(), Eq(stub_surface1.get()));
        });
    };

    auto const async_scene_query = [&]{
        std::async(std::launch::async, scene_query);
    };

    EXPECT_CALL(observer, surface_exists(stub_surface1)).Times(1)
        .WillOnce(InvokeWithoutArgs(async_scene_query));

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_observer(mt::fake_shared(observer));
}


TEST_F(SurfaceStack, scene_observer_can_remove_surface_from_scene_within_surface_exists_notification)
{
    using namespace ::testing;

    MockSceneObserver observer;

    auto const surface_removal = [&]{
        stack.remove_surface(stub_surface1);
    };
    EXPECT_CALL(observer, surface_exists(stub_surface1)).Times(1)
        .WillOnce(InvokeWithoutArgs(surface_removal));

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_observer(mt::fake_shared(observer));
}

TEST_F(SurfaceStack, surfaces_reordered)
{
    using namespace ::testing;

    MockSceneObserver observer;

    EXPECT_CALL(observer, surface_added(_)).Times(AnyNumber());
    EXPECT_CALL(observer, surface_removed(_)).Times(AnyNumber());

    EXPECT_CALL(observer, surfaces_reordered()).Times(1);

    stack.add_observer(mt::fake_shared(observer));

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.raise(stub_surface1);
}

TEST_F(SurfaceStack, scene_elements_hold_snapshot_of_positioning_info)
{
    size_t num_surfaces{3};

    std::vector<std::shared_ptr<ms::Surface>> surfaces;
    for(auto i = 0u; i < num_surfaces; i++)
    {
        auto const surface = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{geom::Point{3 * i, 4 * i},geom::Size{1 * i, 2 * i}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { std::make_shared<mtd::StubBufferStream>(), {}, {} } },
            std::shared_ptr<mg::CursorImage>(),
            report);

        surfaces.emplace_back(surface);
        stack.add_surface(surface, default_params.input_mode);
    }

    auto const elements = stack.scene_elements_for(compositor_id);

    auto const changed_position = geom::Point{43,44};
    for(auto const& surface : surfaces)
        surface->move_to(changed_position);

    //check that the renderables are not at changed_pos
    for(auto& element : elements)
        EXPECT_THAT(changed_position, testing::Ne(element->renderable()->screen_position().top_left));
}

TEST_F(SurfaceStack, generates_scene_elements_that_delay_buffer_acquisition)
{
    using namespace testing;

    auto mock_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    EXPECT_CALL(*mock_stream, lock_compositor_buffer(_))
        .Times(0);

    auto const surface = std::make_shared<ms::BasicSurface>(
        std::string("stub"),
        geom::Rectangle{geom::Point{3, 4},geom::Size{1, 2}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo> { { mock_stream, {}, {} } },
        std::shared_ptr<mg::CursorImage>(),
        report);
        stack.add_surface(surface, default_params.input_mode);

    auto const elements = stack.scene_elements_for(compositor_id);

    Mock::VerifyAndClearExpectations(mock_stream.get());
    EXPECT_CALL(*mock_stream, lock_compositor_buffer(compositor_id))
        .Times(1)
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));
    ASSERT_THAT(elements.size(), Eq(1u));
    elements.front()->renderable()->buffer();
}

TEST_F(SurfaceStack, generates_scene_elements_that_allow_only_one_buffer_acquisition)
{
    using namespace testing;

    auto mock_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    EXPECT_CALL(*mock_stream, lock_compositor_buffer(_))
        .Times(1)
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));

    auto const surface = std::make_shared<ms::BasicSurface>(
        std::string("stub"),
        geom::Rectangle{geom::Point{3, 4},geom::Size{1, 2}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo> { { mock_stream, {}, {} } },
        std::shared_ptr<mg::CursorImage>(),
        report);
        stack.add_surface(surface, default_params.input_mode);

    auto const elements = stack.scene_elements_for(compositor_id);
    ASSERT_THAT(elements.size(), Eq(1u));
    elements.front()->renderable()->buffer();
    elements.front()->renderable()->buffer();
    elements.front()->renderable()->buffer();
}

namespace
{
struct MockConfigureSurface : public ms::BasicSurface
{
    MockConfigureSurface() :
        ms::BasicSurface(
            {},
            {{},{}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { std::make_shared<mtd::StubBufferStream>(), {}, {} } },
            {},
            mir::report::null_scene_report())
    {
    }
    MOCK_METHOD2(configure, int(MirWindowAttrib, int));
};
}

TEST_F(SurfaceStack, occludes_not_rendered_surface)
{
    using namespace testing;

    mc::CompositorID const compositor_id2{&compositor_id};

    stack.register_compositor(compositor_id);
    stack.register_compositor(compositor_id2);

    auto const mock_surface = std::make_shared<MockConfigureSurface>();
    mock_surface->show();
    
    stack.add_surface(mock_surface, default_params.input_mode);

    auto const elements = stack.scene_elements_for(compositor_id);
    ASSERT_THAT(elements.size(), Eq(1u));
    auto const elements2 = stack.scene_elements_for(compositor_id2);
    ASSERT_THAT(elements2.size(), Eq(1u));

    EXPECT_CALL(*mock_surface, configure(mir_window_attrib_visibility, mir_window_visibility_occluded));

    elements.back()->occluded();
    elements2.back()->occluded();
}

TEST_F(SurfaceStack, exposes_rendered_surface)
{
    using namespace testing;

    mc::CompositorID const compositor_id2{&compositor_id};

    stack.register_compositor(compositor_id);
    stack.register_compositor(compositor_id2);

    auto const mock_surface = std::make_shared<MockConfigureSurface>();
        stack.add_surface(mock_surface, default_params.input_mode);

    auto const elements = stack.scene_elements_for(compositor_id);
    ASSERT_THAT(elements.size(), Eq(1u));
    auto const elements2 = stack.scene_elements_for(compositor_id2);
    ASSERT_THAT(elements2.size(), Eq(1u));

    EXPECT_CALL(*mock_surface, configure(mir_window_attrib_visibility, mir_window_visibility_exposed));

    elements.back()->occluded();
    elements2.back()->rendered();
}

TEST_F(SurfaceStack, occludes_surface_when_unregistering_all_compositors_that_rendered_it)
{
    using namespace testing;

    mc::CompositorID const compositor_id2{&compositor_id};
    mc::CompositorID const compositor_id3{&compositor_id2};

    stack.register_compositor(compositor_id);
    stack.register_compositor(compositor_id2);
    stack.register_compositor(compositor_id3);

    auto const mock_surface = std::make_shared<MockConfigureSurface>();
    stack.add_surface(mock_surface, default_params.input_mode);

    auto const elements = stack.scene_elements_for(compositor_id);
    ASSERT_THAT(elements.size(), Eq(1u));
    auto const elements2 = stack.scene_elements_for(compositor_id2);
    ASSERT_THAT(elements2.size(), Eq(1u));
    auto const elements3 = stack.scene_elements_for(compositor_id3);
    ASSERT_THAT(elements3.size(), Eq(1u));

    EXPECT_CALL(*mock_surface, configure(mir_window_attrib_visibility, mir_window_visibility_exposed))
        .Times(2);

    elements.back()->occluded();
    elements2.back()->rendered();
    elements3.back()->rendered();

    Mock::VerifyAndClearExpectations(mock_surface.get());

    EXPECT_CALL(*mock_surface, configure(mir_window_attrib_visibility, mir_window_visibility_occluded));

    stack.unregister_compositor(compositor_id2);
    stack.unregister_compositor(compositor_id3);
}

TEST_F(SurfaceStack, observer_can_trigger_state_change_within_notification)
{
    using namespace ::testing;

    MockSceneObserver observer;

    auto const state_changer = [&]{
        stack.add_surface(stub_surface1, default_params.input_mode);
    };

    //Make sure another thread can also change state
    auto const async_state_changer = [&]{
        std::async(std::launch::async, state_changer);
    };

    EXPECT_CALL(observer, surface_added(stub_surface1)).Times(3)
        .WillOnce(InvokeWithoutArgs(state_changer))
        .WillOnce(InvokeWithoutArgs(async_state_changer))
        .WillOnce(Return());

    stack.add_observer(mt::fake_shared(observer));

    state_changer();
}

TEST_F(SurfaceStack, observer_can_remove_itself_within_notification)
{
    using namespace testing;

    MockSceneObserver observer1;
    MockSceneObserver observer2;
    MockSceneObserver observer3;

    auto const remove_observer = [&]{
        stack.remove_observer(mt::fake_shared(observer2));
    };

    //Both of these observers should still get their notifications
    //regardless of the removal of observer2
    EXPECT_CALL(observer1, surface_added(stub_surface1)).Times(2);
    EXPECT_CALL(observer3, surface_added(stub_surface1)).Times(2);

    InSequence seq;
    EXPECT_CALL(observer2, surface_added(stub_surface1)).Times(1)
         .WillOnce(InvokeWithoutArgs(remove_observer));
    EXPECT_CALL(observer2, end_observation()).Times(1);

    stack.add_observer(mt::fake_shared(observer1));
    stack.add_observer(mt::fake_shared(observer2));
    stack.add_observer(mt::fake_shared(observer3));

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface1, default_params.input_mode);
}

TEST_F(SurfaceStack, scene_observer_notified_of_add_and_remove_input_visualization)
{
    using namespace ::testing;

    MockSceneObserver observer;
    mtd::StubRenderable r;

    InSequence seq;
    EXPECT_CALL(observer, scene_changed()).Times(2);

    stack.add_observer(mt::fake_shared(observer));

    stack.add_input_visualization(mt::fake_shared(r));
    stack.remove_input_visualization(mt::fake_shared(r));
}

TEST_F(SurfaceStack, overlays_do_not_appear_in_input_enumeration)
{
    mtd::StubRenderable r;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    // Configure surface1 and surface2 to appear in input enumeration.
    stub_surface1->configure(mir_window_attrib_visibility, MirWindowVisibility::mir_window_visibility_exposed);
    stub_surface2->configure(mir_window_attrib_visibility, MirWindowVisibility::mir_window_visibility_exposed);

    stack.add_input_visualization(mt::fake_shared(r));

    unsigned int observed_input_targets = 0;
    stack.for_each([&observed_input_targets](std::shared_ptr<mi::Surface> const&)
        {
            observed_input_targets++;
        });
    EXPECT_EQ(2, observed_input_targets);
}

TEST_F(SurfaceStack, overlays_appear_at_top_of_renderlist)
{
    using namespace ::testing;

    mtd::StubRenderable r;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_input_visualization(mt::fake_shared(r));
    stack.add_surface(stub_surface2, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(mt::fake_shared(r))));
}

TEST_F(SurfaceStack, removed_overlays_are_removed)
{
    using namespace ::testing;

    mtd::StubRenderable r;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_input_visualization(mt::fake_shared(r));
    stack.add_surface(stub_surface2, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(mt::fake_shared(r))));

    stack.remove_input_visualization(mt::fake_shared(r));

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2)));
}

TEST_F(SurfaceStack, scene_observers_notified_of_generic_scene_change)
{
    MockSceneObserver o1, o2;

    EXPECT_CALL(o1, scene_changed()).Times(1);
    EXPECT_CALL(o2, scene_changed()).Times(1);

    stack.add_observer(mt::fake_shared(o1));
    stack.add_observer(mt::fake_shared(o2));

    stack.emit_scene_changed();
}

TEST_F(SurfaceStack, for_each_enumerates_all_input_surfaces)
{
    using namespace ::testing;

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    stub_surface1->configure(mir_window_attrib_visibility, MirWindowVisibility::mir_window_visibility_exposed);
    stub_surface2->configure(mir_window_attrib_visibility, MirWindowVisibility::mir_window_visibility_occluded);
    stub_surface3->configure(mir_window_attrib_visibility, MirWindowVisibility::mir_window_visibility_occluded);

    int num_exposed_surfaces = 0;
    auto const count_exposed_surfaces = [&num_exposed_surfaces](std::shared_ptr<mi::Surface> const&){
        num_exposed_surfaces++;
    };

    stack.for_each(count_exposed_surfaces);
    EXPECT_THAT(num_exposed_surfaces, Eq(3));
}

using namespace ::testing;

TEST_F(SurfaceStack, returns_top_surface_under_cursor)
{
    geom::Point const cursor_over_all {100, 100};
    geom::Point const cursor_over_12  {200, 100};
    geom::Point const cursor_over_1   {600, 600};
    geom::Point const cursor_over_none{999, 999};

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    stub_surface1->resize({900, 900});
    stub_surface2->resize({500, 200});
    stub_surface3->resize({200, 500});

    EXPECT_THAT(stack.surface_at(cursor_over_all),  Eq(stub_surface3));
    EXPECT_THAT(stack.surface_at(cursor_over_12),   Eq(stub_surface2));
    EXPECT_THAT(stack.surface_at(cursor_over_1),    Eq(stub_surface1));
    EXPECT_THAT(stack.surface_at(cursor_over_none).get(), IsNull());
}

TEST_F(SurfaceStack, returns_top_visible_surface_under_cursor)
{
    geom::Point const cursor_over_all {100, 100};
    geom::Point const cursor_over_12  {200, 100};
    geom::Point const cursor_over_1   {600, 600};
    geom::Point const cursor_over_none{999, 999};

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);
    stack.add_surface(invisible_stub_surface, default_params.input_mode);

    stub_surface1->resize({900, 900});
    stub_surface2->resize({500, 200});
    stub_surface3->resize({200, 500});
    invisible_stub_surface->resize({999, 999});

    EXPECT_THAT(stack.surface_at(cursor_over_all),  Eq(stub_surface3));
    EXPECT_THAT(stack.surface_at(cursor_over_12),   Eq(stub_surface2));
    EXPECT_THAT(stack.surface_at(cursor_over_1),    Eq(stub_surface1));
    EXPECT_THAT(stack.surface_at(cursor_over_none).get(), IsNull());
}

TEST_F(SurfaceStack, raise_surfaces_to_top)
{
    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    NiceMock<MockSceneObserver> observer;
    stack.add_observer(mt::fake_shared(observer));

    EXPECT_CALL(observer, surfaces_reordered()).Times(1);

    stack.raise({stub_surface1, stub_surface3});
    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream3)));

    Mock::VerifyAndClearExpectations(&observer);
    EXPECT_CALL(observer, surfaces_reordered()).Times(1);

    stack.raise({stub_surface2, stub_surface3});
    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream3)));

    Mock::VerifyAndClearExpectations(&observer);
    EXPECT_CALL(observer, surfaces_reordered()).Times(0);

    stack.raise({stub_surface2, stub_surface1, stub_surface3});
    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream3)));
}

TEST_F(SurfaceStack, new_surface_is_placed_under_old_according_to_depth_layer)
{
    stub_surface1->set_depth_layer(mir_depth_layer_application);
    stub_surface2->set_depth_layer(mir_depth_layer_below);

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream1)));
}

TEST_F(SurfaceStack, raise_does_not_reorder_surfaces_when_depth_layers_are_different)
{
    stub_surface1->set_depth_layer(mir_depth_layer_below);
    stub_surface2->set_depth_layer(mir_depth_layer_application);
    stub_surface3->set_depth_layer(mir_depth_layer_above);

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    NiceMock<MockSceneObserver> observer;
    stack.add_observer(mt::fake_shared(observer));

    EXPECT_CALL(observer, surfaces_reordered()).Times(0);

    stack.raise({stub_surface1, stub_surface3});
    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream3)));
}

TEST_F(SurfaceStack, changing_depth_layer_causes_reorder)
{
    NiceMock<MockSceneObserver> observer;
    stack.add_observer(mt::fake_shared(observer));

    EXPECT_CALL(observer, surfaces_reordered()).Times(0);

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2)));

    Mock::VerifyAndClearExpectations(&observer);
    EXPECT_CALL(observer, surfaces_reordered()).Times(1);

    stub_surface1->set_depth_layer(mir_depth_layer_above);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream1)));
}

TEST_F(SurfaceStack, changing_depth_layer_does_not_cause_reorder_when_it_shouldnt)
{
    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2)));

    stub_surface1->set_depth_layer(mir_depth_layer_below);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2)));
}

TEST_F(SurfaceStack, raising_surface_set_respects_depth_layers)
{
    stub_surface1->set_depth_layer(mir_depth_layer_application);
    stub_surface2->set_depth_layer(mir_depth_layer_above);
    stub_surface3->set_depth_layer(mir_depth_layer_above);

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.input_mode);

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream2),
            SceneElementForStream(stub_buffer_stream3)));

    NiceMock<MockSceneObserver> observer;
    stack.add_observer(mt::fake_shared(observer));

    EXPECT_CALL(observer, surfaces_reordered()).Times(1);

    stack.raise({stub_surface1, stub_surface2});

    EXPECT_THAT(
        stack.scene_elements_for(compositor_id),
        ElementsAre(
            SceneElementForStream(stub_buffer_stream1),
            SceneElementForStream(stub_buffer_stream3),
            SceneElementForStream(stub_buffer_stream2)));
}

TEST_F(SurfaceStack, all_depth_layers_are_handled)
{
    std::vector<MirDepthLayer> depth_layers_in_order{
        mir_depth_layer_background,
        mir_depth_layer_below,
        mir_depth_layer_application,
        mir_depth_layer_always_on_top,
        mir_depth_layer_above,
        mir_depth_layer_overlay,
    };

    std::vector<std::string> depth_layer_names;

    for (auto layer : depth_layers_in_order)
    {
        switch (layer)
        {
        case mir_depth_layer_background:
            depth_layer_names.push_back("mir_depth_layer_background");
            break;
        case mir_depth_layer_below:
            depth_layer_names.push_back("mir_depth_layer_below");
            break;
        case mir_depth_layer_application:
            depth_layer_names.push_back("mir_depth_layer_application");
            break;
        case mir_depth_layer_always_on_top:
            depth_layer_names.push_back("mir_depth_layer_always_on_top");
            break;
        case mir_depth_layer_above:
            depth_layer_names.push_back("mir_depth_layer_above");
            break;
        case mir_depth_layer_overlay:
            depth_layer_names.push_back("mir_depth_layer_overlay");
            break;
        // NOTE: when adding a depth layer, be sure to add it to depth_layers_in_order
        // (This switch mostly exists for that reminder)
        }
    }

    stub_surface1->set_depth_layer(depth_layers_in_order[0]);
    stub_surface2->set_depth_layer(depth_layers_in_order[0]);

    stack.add_surface(stub_surface1, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.input_mode);

    for (auto i = 1u; i < depth_layers_in_order.size(); i++)
    {
        stub_surface1->set_depth_layer(depth_layers_in_order[i]);
        stack.raise(stub_surface2);

        EXPECT_THAT(
            stack.scene_elements_for(compositor_id),
            ElementsAre(
                SceneElementForStream(stub_buffer_stream2),
                SceneElementForStream(stub_buffer_stream1)))
            << depth_layer_names[i] << " not kept on top of " << depth_layer_names[i - 1];

        stub_surface2->set_depth_layer(depth_layers_in_order[i]);

        EXPECT_THAT(
            stack.scene_elements_for(compositor_id),
            ElementsAre(
                SceneElementForStream(stub_buffer_stream1),
                SceneElementForStream(stub_buffer_stream2)))
            << "A surface at " << depth_layer_names[i] << " could not be raised over another on the same layer";
    }

}
