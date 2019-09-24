/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */


#include "src/server/scene/basic_surface.h"
#include "src/server/scene/legacy_surface_change_notification.h"

#include "mir/events/event_private.h"
#include "mir/frontend/event_sink.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/displacement.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/events/event_builders.h"

#include "mir/test/doubles/stub_cursor_image.h"
#include "mir/test/doubles/mock_buffer_stream.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/fake_shared.h"

#include "src/server/report/null_report_factory.h"

#include <algorithm>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mr = mir::report;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;

namespace
{
class MockCallback
{
public:
    MOCK_METHOD0(call, void());
};

class MockSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    MOCK_METHOD3(attrib_changed, void(ms::Surface const*, MirWindowAttrib, int));
    MOCK_METHOD2(hidden_set_to, void(ms::Surface const*, bool));
    MOCK_METHOD2(renamed, void(ms::Surface const*, char const*));
    MOCK_METHOD1(client_surface_close_requested, void(ms::Surface const*));
    MOCK_METHOD2(cursor_image_set_to, void(ms::Surface const*, mir::graphics::CursorImage const& image));
    MOCK_METHOD1(cursor_image_removed, void(ms::Surface const*));
};

struct BasicSurfaceTest : public testing::Test
{
    std::string const name{"aa"};
    geom::Rectangle const rect{{4,7},{5,9}};

    testing::NiceMock<MockCallback> mock_callback;
    std::function<void()> null_change_cb{[]{}};
    std::function<void()> mock_change_cb{std::bind(&MockCallback::call, &mock_callback)};
    std::shared_ptr<testing::NiceMock<mtd::MockBufferStream>> mock_buffer_stream =
        std::make_shared<testing::NiceMock<mtd::MockBufferStream>>();
    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    void const* compositor_id{nullptr};
    std::shared_ptr<ms::LegacySurfaceChangeNotification> observer =
        std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb, [this](int){mock_change_cb();});
    std::list<ms::StreamInfo> streams { { mock_buffer_stream, {}, {} } };

    ms::BasicSurface surface{
        name,
        rect,
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report};

    BasicSurfaceTest()
    {
        // use an opaque pixel format by default
        ON_CALL(*mock_buffer_stream, pixel_format())
            .WillByDefault(testing::Return(mir_pixel_format_xrgb_8888));
    }
};

}

TEST_F(BasicSurfaceTest, basics)
{
    EXPECT_EQ(name, surface.name());
    EXPECT_EQ(rect.size, surface.size());
    EXPECT_EQ(rect.top_left, surface.top_left());
    for (auto& renderable : surface.generate_renderables(this))
        EXPECT_FALSE(renderable->shaped());
}

TEST_F(BasicSurfaceTest, buffer_stream_ids_always_unique)
{
    int const n = 10;
    std::array<std::unique_ptr<ms::BasicSurface>, n> surfaces;

    std::multiset<mg::Renderable::ID> ids;
    for (auto& surface : surfaces)
    {
        surface = std::make_unique<ms::BasicSurface>(
                name, rect, mir_pointer_unconfined,
                std::list<ms::StreamInfo> {
                    { std::make_shared<testing::NiceMock<mtd::MockBufferStream>>(), {}, {} } },
                std::shared_ptr<mg::CursorImage>(), report);
        for (auto& renderable : surface->generate_renderables(this))
            ids.insert(renderable->id());
    }
    for (auto& it : ids)
        EXPECT_THAT(ids.count(it), testing::Eq(1));
}

TEST_F(BasicSurfaceTest, id_never_invalid)
{
    int const n = 10;
    std::array<std::unique_ptr<ms::BasicSurface>, n> surfaces;

    std::multiset<mg::Renderable::ID> ids;
    for (auto& surface : surfaces)
    {
        surface = std::make_unique<ms::BasicSurface>(
                name, rect, mir_pointer_unconfined, streams,
                std::shared_ptr<mg::CursorImage>(), report);

        for (auto& renderable : surface->generate_renderables(this))
            EXPECT_THAT(renderable->id(), testing::Ne(nullptr));
    }
}

TEST_F(BasicSurfaceTest, update_top_left)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    surface.add_observer(observer);

    EXPECT_EQ(rect.top_left, surface.top_left());

    auto new_top_left = geom::Point{geom::X{6}, geom::Y{10}};
    surface.move_to(new_top_left);
    EXPECT_EQ(new_top_left, surface.top_left());
}

TEST_F(BasicSurfaceTest, update_size)
{
    geom::Size const new_size{34, 56};

    EXPECT_CALL(mock_callback, call())
        .Times(1);

    surface.add_observer(observer);

    EXPECT_EQ(rect.size, surface.size());
    EXPECT_NE(new_size, surface.size());

    auto renderables = surface.generate_renderables(compositor_id);
    ASSERT_THAT(renderables.size(), testing::Eq(1));
    auto old_transformation = renderables[0]->transformation();

    surface.resize(new_size);
    EXPECT_EQ(new_size, surface.size());
    // Size no longer affects transformation:
    renderables = surface.generate_renderables(compositor_id);
    ASSERT_THAT(renderables.size(), testing::Eq(1));
    EXPECT_THAT(renderables[0]->transformation(), testing::Eq(old_transformation));
}

/*
 * Until logic is implemented to separate size() from client_size(), verify
 * they do return the same thing for backward compatibility.
 */
TEST_F(BasicSurfaceTest, size_equals_client_size)
{
    geom::Size const new_size{34, 56};

    EXPECT_EQ(rect.size, surface.size());
    EXPECT_EQ(rect.size, surface.client_size());
    EXPECT_NE(new_size, surface.size());
    EXPECT_NE(new_size, surface.client_size());

    surface.resize(new_size);
    EXPECT_EQ(new_size, surface.size());
    EXPECT_EQ(new_size, surface.client_size());
}

TEST_F(BasicSurfaceTest, test_surface_set_transformation_updates_transform)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    surface.add_observer(observer);

    auto renderables = surface.generate_renderables(compositor_id);
    ASSERT_THAT(renderables.size(), testing::Eq(1));
    auto original_transformation = renderables[0]->transformation();
    glm::mat4 trans{0.1f, 0.5f, 0.9f, 1.3f,
                    0.2f, 0.6f, 1.0f, 1.4f,
                    0.3f, 0.7f, 1.1f, 1.5f,
                    0.4f, 0.8f, 1.2f, 1.6f};

    surface.set_transformation(trans);
    renderables = surface.generate_renderables(compositor_id);
    ASSERT_THAT(renderables.size(), testing::Eq(1));
    auto got = renderables[0]->transformation();
    EXPECT_NE(original_transformation, got);
    EXPECT_EQ(trans, got);
}

TEST_F(BasicSurfaceTest, test_surface_is_opaque_by_default)
{
    using namespace testing;

    auto renderables = surface.generate_renderables(compositor_id);
    ASSERT_THAT(renderables.size(), testing::Eq(1));
    EXPECT_FALSE(renderables[0]->shaped());
}

TEST_F(BasicSurfaceTest, test_surface_visibility)
{
    using namespace testing;
    mtd::StubBuffer mock_buffer;
    auto submitted_buffer = false;
    ON_CALL(*mock_buffer_stream, has_submitted_buffer())
        .WillByDefault(Invoke([&submitted_buffer] { return submitted_buffer; }));

    // Must be a fresh surface to guarantee no frames posted yet...
    ms::BasicSurface surface{
        name,
        rect,
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report};

    //not visible by default
    EXPECT_FALSE(surface.visible());

    surface.set_hidden(false);
    //not renderable if no first frame has been posted by client, regardless of hide state
    EXPECT_FALSE(surface.visible());
    surface.set_hidden(true);
    EXPECT_FALSE(surface.visible());

    surface.set_hidden(false);
    submitted_buffer = true;
    EXPECT_TRUE(surface.visible());
}

TEST_F(BasicSurfaceTest, test_surface_hidden_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    surface.add_observer(observer);

    surface.set_hidden(true);
}

// a 1x1 window at (1,1) will get events at (1,1)
TEST_F(BasicSurfaceTest, default_region_is_surface_rectangle)
{
    geom::Point pt(1,1);
    geom::Size one_by_one{geom::Width{1}, geom::Height{1}};
    ms::BasicSurface surface{
        name,
        geom::Rectangle{pt, one_by_one},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report};

    surface.add_observer(observer);

    std::vector<geom::Point> contained_pt
    {
        geom::Point{geom::X{1}, geom::Y{1}}
    };

    for(auto x = 0; x <= 3; x++)
    {
        for(auto y = 0; y <= 3; y++)
        {
            auto test_pt = geom::Point{x, y};
            auto contains = surface.input_area_contains(test_pt);
            if (std::find(contained_pt.begin(), contained_pt.end(), test_pt) != contained_pt.end())
            {
                EXPECT_TRUE(contains);
            }
            else
            {
                EXPECT_FALSE(contains);
            }
        }
    }
}

TEST_F(BasicSurfaceTest, default_invisible_surface_doesnt_get_input)
{
    ms::BasicSurface surface{
        name,
        geom::Rectangle{{0,0}, {100,100}},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report};

    EXPECT_CALL(*mock_buffer_stream, has_submitted_buffer())
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));

    EXPECT_FALSE(surface.input_area_contains({50,50}));
    EXPECT_TRUE(surface.input_area_contains({50,50}));
}

TEST_F(BasicSurfaceTest, surface_doesnt_get_input_outside_clip_area)
{
    ms::BasicSurface surface{
        name,
        geom::Rectangle{{0,0}, {100,100}},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report};

    surface.set_clip_area(std::experimental::optional<geom::Rectangle>({{0,0}, {50,50}}));

    EXPECT_FALSE(surface.input_area_contains({75,75}));
    EXPECT_TRUE(surface.input_area_contains({25,25}));

    surface.set_clip_area(std::experimental::optional<geom::Rectangle>());

    EXPECT_TRUE(surface.input_area_contains({75,75}));
}

TEST_F(BasicSurfaceTest, set_input_region)
{
    std::vector<geom::Rectangle> const rectangles = {
        {{geom::X{0}, geom::Y{0}}, {geom::Width{1}, geom::Height{1}}}, //region0
        {{geom::X{1}, geom::Y{1}}, {geom::Width{1}, geom::Height{1}}}  //region1
    };

    surface.add_observer(observer);

    surface.set_input_region(rectangles);

    std::vector<geom::Point> contained_pt
    {
        //region0 points
        geom::Point{geom::X{4}, geom::Y{7}},
        //region1 points
        geom::Point{geom::X{5}, geom::Y{8}},
    };

    for(auto x = 0; x <= 3; x++)
    {
        for(auto y = 0; y <= 3; y++)
        {
            auto test_pt = rect.top_left + geom::Displacement{x, y};
            auto contains = surface.input_area_contains(test_pt);
            if (std::find(contained_pt.begin(), contained_pt.end(), test_pt) != contained_pt.end())
            {
                EXPECT_TRUE(contains);
            }
            else
            {
                EXPECT_FALSE(contains);
            }
        }
    }
}

TEST_F(BasicSurfaceTest, updates_default_input_region_when_surface_is_resized_to_larger_size)
{
    geom::Rectangle const new_rect{rect.top_left,{10,10}};
    surface.resize(new_rect.size);

    for (auto x = new_rect.top_left.x.as_int() - 1;
         x <= new_rect.top_right().x.as_int();
         x++)
    {
        for (auto y = new_rect.top_left.y.as_int() - 1;
             y <= new_rect.bottom_left().y.as_int();
             y++)
        {
            auto const test_pt = geom::Point{x, y};
            auto const contains = surface.input_area_contains(test_pt);
            if (new_rect.contains(test_pt))
            {
                EXPECT_TRUE(contains) << " point = " << test_pt;
            }
            else
            {
                EXPECT_FALSE(contains) << " point = " << test_pt;
            }
        }
    }
}

TEST_F(BasicSurfaceTest, updates_default_input_region_when_surface_is_resized_to_smaller_size)
{
    geom::Rectangle const new_rect{rect.top_left,{2,2}};
    surface.resize(new_rect.size);

    for (auto x = rect.top_left.x.as_int() - 1;
         x <= rect.top_right().x.as_int();
         x++)
    {
        for (auto y = rect.top_left.y.as_int() - 1;
             y <= rect.bottom_left().y.as_int();
             y++)
        {
            auto const test_pt = geom::Point{x, y};
            auto const contains = surface.input_area_contains(test_pt);
            if (new_rect.contains(test_pt))
            {
                EXPECT_TRUE(contains) << " point = " << test_pt;
            }
            else
            {
                EXPECT_FALSE(contains) << " point = " << test_pt;
            }
        }
    }
}

TEST_F(BasicSurfaceTest, restores_default_input_region_when_setting_empty_input_region)
{
    std::vector<geom::Rectangle> const rectangles = {
        {{geom::X{0}, geom::Y{0}}, {geom::Width{1}, geom::Height{1}}}, //region0
    };

    surface.set_input_region(rectangles);
    EXPECT_FALSE(surface.input_area_contains(rect.bottom_right() - geom::Displacement{1,1}));

    surface.set_input_region({});
    EXPECT_TRUE(surface.input_area_contains(rect.bottom_right() - geom::Displacement{1,1}));
}

TEST_F(BasicSurfaceTest, disables_input_when_setting_input_region_with_empty_rectangle)
{
    surface.set_input_region({geom::Rectangle()});
    EXPECT_FALSE(surface.input_area_contains(rect.top_left));
}

TEST_F(BasicSurfaceTest, reception_mode_is_normal_by_default)
{
    EXPECT_EQ(mi::InputReceptionMode::normal, surface.reception_mode());
}

TEST_F(BasicSurfaceTest, reception_mode_can_be_changed)
{
    surface.set_reception_mode(mi::InputReceptionMode::receives_all_input);

    EXPECT_EQ(mi::InputReceptionMode::receives_all_input, surface.reception_mode());
}

TEST_F(BasicSurfaceTest, stores_parent)
{
    auto parent = mt::fake_shared(surface);
    ms::BasicSurface child{
        name,
        geom::Rectangle{{0,0}, {100,100}},
        parent,
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report};

    EXPECT_EQ(child.parent(), parent);
}

namespace
{

struct AttributeTestParameters
{
    MirWindowAttrib attribute;
    int default_value;
    int a_valid_value;
    int infimum_invalid_value;
    int supremum_invalid_value;
};

struct BasicSurfaceAttributeTest : public BasicSurfaceTest,
    public ::testing::WithParamInterface<AttributeTestParameters>
{
};

AttributeTestParameters const surface_visibility_test_parameters{
    mir_window_attrib_visibility,
    mir_window_visibility_occluded,
    mir_window_visibility_exposed,
    mir_window_visibility_exposed + 1,
    mir_window_visibility_occluded - 1
};

AttributeTestParameters const surface_type_test_parameters{
    mir_window_attrib_type,
    mir_window_type_normal,
    mir_window_type_freestyle,
    mir_window_types,
    mir_window_type_normal - 1
};

AttributeTestParameters const surface_state_test_parameters{
    mir_window_attrib_state,
    mir_window_state_restored,
    mir_window_state_fullscreen,
    mir_window_types,
    mir_window_state_unknown - 1
};

AttributeTestParameters const surface_swapinterval_test_parameters{
    mir_window_attrib_swapinterval,
    1,
    0,
    -1,
    -1
};

AttributeTestParameters const surface_dpi_test_parameters{
    mir_window_attrib_dpi,
    0,
    90,
    -1,
    -1
};

AttributeTestParameters const surface_focus_test_parameters{
    mir_window_attrib_focus,
    mir_window_focus_state_unfocused,
    mir_window_focus_state_focused,
    mir_window_focus_state_focused + 1,
    mir_window_focus_state_unfocused - 1
};

}

TEST_P(BasicSurfaceAttributeTest, default_value)
{
    auto const& params = GetParam();
    auto const& attribute = params.attribute;
    auto const& default_value = params.default_value;
    
    EXPECT_EQ(default_value, surface.query(attribute));
}

TEST_P(BasicSurfaceAttributeTest, notifies_about_attrib_changes)
{
    using namespace testing;

    auto const& params = GetParam();
    auto const& attribute = params.attribute;
    auto const& value1 = params.a_valid_value;
    auto const& value2 = params.default_value;

    NiceMock<MockSurfaceObserver> mock_surface_observer;

    InSequence s;
    EXPECT_CALL(mock_surface_observer, attrib_changed(_, attribute, value1))
        .Times(1);
    EXPECT_CALL(mock_surface_observer, attrib_changed(_, attribute, value2))
        .Times(1);

    surface.add_observer(mt::fake_shared(mock_surface_observer));

    surface.configure(attribute, value1);
    surface.configure(attribute, value2);
}

TEST_P(BasicSurfaceAttributeTest, does_not_notify_if_attrib_is_unchanged)
{
    using namespace testing;

    auto const& params = GetParam();
    auto const& attribute = params.attribute;
    auto const& default_value = params.default_value;
    auto const& another_value = params.a_valid_value;

    NiceMock<MockSurfaceObserver> mock_surface_observer;

    EXPECT_CALL(mock_surface_observer, attrib_changed(_, attribute, another_value))
        .Times(1);

    surface.add_observer(mt::fake_shared(mock_surface_observer));

    surface.configure(attribute, default_value);
    surface.configure(attribute, another_value);
    surface.configure(attribute, another_value);
}

TEST_P(BasicSurfaceAttributeTest, throws_on_invalid_value)
{
    using namespace testing;
    
    auto const& params = GetParam();
    auto const& attribute = params.attribute;
    auto const& invalid_value = params.infimum_invalid_value;
    
    EXPECT_THROW({
            surface.configure(attribute, invalid_value);
        }, std::logic_error);
    EXPECT_THROW({
            surface.configure(attribute, params.supremum_invalid_value);
        }, std::logic_error);
}

INSTANTIATE_TEST_CASE_P(SurfaceTypeAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_type_test_parameters));

INSTANTIATE_TEST_CASE_P(SurfaceVisibilityAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_visibility_test_parameters));

INSTANTIATE_TEST_CASE_P(SurfaceStateAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_state_test_parameters));

INSTANTIATE_TEST_CASE_P(SurfaceSwapintervalAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_swapinterval_test_parameters));

INSTANTIATE_TEST_CASE_P(SurfaceDPIAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_dpi_test_parameters));

INSTANTIATE_TEST_CASE_P(SurfaceFocusAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_focus_test_parameters));

TEST_F(BasicSurfaceTest, notifies_about_cursor_image_change)
{
    using namespace testing;

    NiceMock<MockSurfaceObserver> mock_surface_observer;

    auto cursor_image = std::make_shared<mtd::StubCursorImage>();
    EXPECT_CALL(mock_surface_observer, cursor_image_set_to(_, _));

    surface.add_observer(mt::fake_shared(mock_surface_observer));
    surface.set_cursor_image(cursor_image);
}

TEST_F(BasicSurfaceTest, notifies_about_cursor_image_removal)
{
    using namespace testing;

    NiceMock<MockSurfaceObserver> mock_surface_observer;

    EXPECT_CALL(mock_surface_observer, cursor_image_set_to(_, _)).Times(0);
    EXPECT_CALL(mock_surface_observer, cursor_image_removed(_));

    surface.add_observer(mt::fake_shared(mock_surface_observer));
    surface.set_cursor_image({});
}

TEST_F(BasicSurfaceTest, notifies_when_cursor_stream_set)
{
    using namespace testing;

    NiceMock<MockSurfaceObserver> mock_surface_observer;
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto stub_buffer = std::make_shared<mtd::StubBuffer>();

    ON_CALL(*buffer_stream, buffers_ready_for_compositor(_))
        .WillByDefault(Return(1));
    ON_CALL(*buffer_stream, lock_compositor_buffer(_))
        .WillByDefault(Return(stub_buffer));
    EXPECT_CALL(mock_surface_observer, cursor_image_set_to(_, _));

    surface.add_observer(mt::fake_shared(mock_surface_observer));
    surface.set_cursor_stream(buffer_stream, {});
}

TEST_F(BasicSurfaceTest, notifies_about_cursor_removal_when_empty_stream_set)
{
    using namespace testing;

    NiceMock<MockSurfaceObserver> mock_surface_observer;
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();

    ON_CALL(*buffer_stream, has_submitted_buffer())
        .WillByDefault(Return(false));
    EXPECT_CALL(mock_surface_observer, cursor_image_set_to(_, _)).Times(0);
    EXPECT_CALL(mock_surface_observer, cursor_image_removed(_));

    surface.add_observer(mt::fake_shared(mock_surface_observer));
    surface.set_cursor_stream(buffer_stream, {});
}

TEST_F(BasicSurfaceTest, cursor_can_be_set_from_stream_that_started_empty)
{
    using namespace testing;

    NiceMock<MockSurfaceObserver> mock_surface_observer;
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::shared_ptr<mtd::StubBuffer> stub_buffer;
    // Must be a shared pointer, because it is set by CursorStreamImageAdapter::reset() in the destructor
    auto frame_posted_callback = std::make_shared<std::function<void(mir::geometry::Size const&)>>([](auto)
        {
            FAIL() << "frame_posted_callback should have been set by the surface";
        });

    ON_CALL(*buffer_stream, buffers_ready_for_compositor(_))
        .WillByDefault(Invoke([&](auto)
            {
                return stub_buffer != nullptr;
            }));
    ON_CALL(*buffer_stream, has_submitted_buffer())
        .WillByDefault(Invoke([&]
            {
                return stub_buffer != nullptr;
            }));
    ON_CALL(*buffer_stream, lock_compositor_buffer(_))
        .WillByDefault(Invoke([&](auto)
            {
                return stub_buffer;
            }));
    ON_CALL(*buffer_stream, set_frame_posted_callback(_))
        .WillByDefault(Invoke([=](auto const& callback)
            {
                *frame_posted_callback = callback;
            }));
    EXPECT_CALL(mock_surface_observer, cursor_image_removed(_)).Times(1);
    EXPECT_CALL(mock_surface_observer, cursor_image_set_to(_, _)).Times(1);

    surface.add_observer(mt::fake_shared(mock_surface_observer));
    surface.set_cursor_stream(buffer_stream, {});
    stub_buffer = std::make_shared<mtd::StubBuffer>();
    (*frame_posted_callback)({});
}

TEST_F(BasicSurfaceTest, observer_can_trigger_state_change_within_notification)
{
    using namespace testing;

    auto const state_changer = [&]{
       surface.set_hidden(false);
    };

    //Make sure another thread can also change state
    auto const async_state_changer = [&]{
        std::async(std::launch::async, state_changer);
    };

    EXPECT_CALL(mock_callback, call()).Times(3)
        .WillOnce(InvokeWithoutArgs(state_changer))
        .WillOnce(InvokeWithoutArgs(async_state_changer))
        .WillOnce(Return());

    surface.add_observer(observer);

    surface.set_hidden(true);
}

TEST_F(BasicSurfaceTest, observer_can_remove_itself_within_notification)
{
    using namespace testing;
    MockSurfaceObserver observer1;
    MockSurfaceObserver observer2;
    MockSurfaceObserver observer3;

    //Both of these observers should still get their notifications
    //regardless of the unregistration of observer2
    EXPECT_CALL(observer1, hidden_set_to(_, true)).Times(2);
    EXPECT_CALL(observer3, hidden_set_to(_, true)).Times(2);

    auto const remove_observer = [&]{
        surface.remove_observer(mt::fake_shared(observer2));
    };

    EXPECT_CALL(observer2, hidden_set_to(_, true)).Times(1)
        .WillOnce(InvokeWithoutArgs(remove_observer));

    surface.add_observer(mt::fake_shared(observer1));
    surface.add_observer(mt::fake_shared(observer2));
    surface.add_observer(mt::fake_shared(observer3));

    surface.set_hidden(true);
    surface.set_hidden(true);
}

TEST_F(BasicSurfaceTest, notifies_of_client_close_request)
{
    using namespace testing;

    MockSurfaceObserver mock_surface_observer;

    EXPECT_CALL(mock_surface_observer, client_surface_close_requested(_)).Times(1);

    surface.add_observer(mt::fake_shared(mock_surface_observer));

    surface.request_client_surface_close();
}

TEST_F(BasicSurfaceTest, notifies_of_rename)
{
    using namespace testing;

    MockSurfaceObserver mock_surface_observer;
    surface.add_observer(mt::fake_shared(mock_surface_observer));

    EXPECT_CALL(mock_surface_observer, renamed(_, StrEq("Steve")));

    surface.rename("Steve");
}

MATCHER_P(IsRenderableOfPosition, pos, "is renderable with position")
{
    return (pos == arg->screen_position().top_left);
}

MATCHER_P(IsRenderableOfSize, size, "is renderable with size")
{
    return (size == arg->screen_position().size);
}

MATCHER_P(IsRenderableOfAlpha, alpha, "is renderable with alpha")
{
    EXPECT_THAT(static_cast<float>(alpha), testing::FloatEq(arg->alpha()));
    return !(::testing::Test::HasFailure());
}

TEST_F(BasicSurfaceTest, adds_buffer_streams)
{
    using namespace testing;
    geom::Displacement d0{19,99};
    geom::Displacement d1{21,101};
    geom::Displacement d2{20,9};
    auto buffer_stream0 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream2 = std::make_shared<NiceMock<mtd::MockBufferStream>>();

    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {}},
        { buffer_stream0, d0, {} },
        { buffer_stream1, d1, {} },
        { buffer_stream2, d2, {} }
    };
    surface.set_streams(streams);

    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(4));
    EXPECT_THAT(renderables[0], IsRenderableOfPosition(rect.top_left));
    EXPECT_THAT(renderables[1], IsRenderableOfPosition(rect.top_left + d0));
    EXPECT_THAT(renderables[2], IsRenderableOfPosition(rect.top_left + d1));
    EXPECT_THAT(renderables[3], IsRenderableOfPosition(rect.top_left + d2));

}

TEST_F(BasicSurfaceTest, moving_surface_repositions_all_associated_streams)
{
    using namespace testing;
    geom::Point pt{10, 20};
    geom::Displacement d{19,99};
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();

    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {} },
        { buffer_stream, d, {} } 
    };

    surface.set_streams(streams);
    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(2));
    EXPECT_THAT(renderables[0], IsRenderableOfPosition(rect.top_left));
    EXPECT_THAT(renderables[1], IsRenderableOfPosition(rect.top_left + d));

    surface.move_to(pt);

    renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(2));
    EXPECT_THAT(renderables[0], IsRenderableOfPosition(pt));
    EXPECT_THAT(renderables[1], IsRenderableOfPosition(pt + d));
}

TEST_F(BasicSurfaceTest, can_remove_all_streams)
{
    using namespace testing;
    surface.set_streams({});
    auto renderables = surface.generate_renderables(this);
    EXPECT_THAT(renderables.size(), Eq(0));
}

TEST_F(BasicSurfaceTest, can_set_streams_not_containing_originally_created_with_stream)
{
    using namespace testing;

    auto buffer_stream0 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { buffer_stream0, {0,0}, {} },
        { buffer_stream1, {0,0}, {} }
    };
    surface.set_streams(streams);
    auto renderables = surface.generate_renderables(this);
    EXPECT_THAT(renderables.size(), Eq(2));
}

TEST_F(BasicSurfaceTest, does_not_render_if_outside_of_clip_area)
{
    using namespace testing;
    geom::Displacement d0{19,99};
    geom::Displacement d1{21,101};
    geom::Displacement d2{20,9};
    auto buffer_stream0 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream2 = std::make_shared<NiceMock<mtd::MockBufferStream>>();

    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {}},
        { buffer_stream0, d0, {} },
        { buffer_stream1, d1, {} },
        { buffer_stream2, d2, {} }
    };
    surface.set_streams(streams);
    surface.set_clip_area(std::experimental::optional<geom::Rectangle>({{200,0},{100,100}}));

    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(0));

    surface.set_clip_area(std::experimental::optional<geom::Rectangle>({{0,0},{100,100}}));

    renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(4));

}

TEST_F(BasicSurfaceTest, registers_frame_callbacks_on_construction)
{
    using namespace testing;

    auto buffer_stream0 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { buffer_stream0, {0,0}, {} },
        { buffer_stream1, {0,0}, {} }
    };

    EXPECT_CALL(*buffer_stream0, set_frame_posted_callback(_));
    EXPECT_CALL(*buffer_stream1, set_frame_posted_callback(_));

    ms::BasicSurface child{
        name,
        geom::Rectangle{{0,0}, {100,100}},
        std::weak_ptr<ms::Surface>{},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report};
}

TEST_F(BasicSurfaceTest, registers_frame_callbacks_on_set_streams)
{
    using namespace testing;

    auto buffer_stream0 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { buffer_stream0, {0,0}, {} },
        { buffer_stream1, {0,0}, {} }
    };

    EXPECT_CALL(*buffer_stream0, set_frame_posted_callback(_));
    EXPECT_CALL(*buffer_stream1, set_frame_posted_callback(_));

    surface.set_streams(streams);
}

TEST_F(BasicSurfaceTest, showing_brings_all_streams_up_to_date)
{
    using namespace testing;
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {} },
        { buffer_stream, {0,0}, {} }
    };
    surface.set_streams(streams);

    EXPECT_CALL(*buffer_stream, drop_old_buffers()).Times(Exactly(1));
    EXPECT_CALL(*mock_buffer_stream, drop_old_buffers()).Times(Exactly(1));

    surface.configure(mir_window_attrib_visibility, mir_window_visibility_occluded);
    surface.configure(mir_window_attrib_visibility, mir_window_visibility_exposed);
    surface.configure(mir_window_attrib_visibility, mir_window_visibility_exposed);
}

//TODO: per-stream alpha and swapinterval seems useful
TEST_F(BasicSurfaceTest, changing_alpha_effects_all_streams)
{
    using namespace testing;
    auto alpha = 0.3;
    
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {} },
        { buffer_stream, {0,0}, {} }
    };

    surface.set_streams(streams);
    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(2));
    EXPECT_THAT(renderables[0], IsRenderableOfAlpha(1.0f));
    EXPECT_THAT(renderables[1], IsRenderableOfAlpha(1.0f));

    surface.set_alpha(alpha);
    renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(2));
    EXPECT_THAT(renderables[0], IsRenderableOfAlpha(alpha));
    EXPECT_THAT(renderables[1], IsRenderableOfAlpha(alpha));
}

TEST_F(BasicSurfaceTest, setting_streams_with_size_changes_sizes)
{
    using namespace testing;
   
    geom::Size size0 {100, 25 };
    geom::Size size1 { 32, 44 };
    geom::Size bad_size { 12, 11 }; 
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    ON_CALL(*mock_buffer_stream, stream_size())
        .WillByDefault(Return(bad_size));
    ON_CALL(*buffer_stream, stream_size())
        .WillByDefault(Return(bad_size));
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, size0 },
        { buffer_stream, {0,0}, size1 }
    };

    surface.set_streams(streams);
    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(2));
    EXPECT_THAT(renderables[0], IsRenderableOfSize(size0));
    EXPECT_THAT(renderables[1], IsRenderableOfSize(size1));
}

TEST_F(BasicSurfaceTest, changing_inverval_effects_all_streams)
{
    using namespace testing;
    
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {} },
        { buffer_stream, {0,0}, {} }
    };

    EXPECT_CALL(*mock_buffer_stream, allow_framedropping(true));
    EXPECT_CALL(*buffer_stream, allow_framedropping(true));

    surface.set_streams(streams);
    surface.configure(mir_window_attrib_swapinterval, 0);
}

TEST_F(BasicSurfaceTest, visibility_matches_produced_list)
{
    using namespace testing;
    auto stream1_visible = false;
    auto stream2_visible = false;
    geom::Displacement displacement{3,-2};
    auto mock_buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    ON_CALL(*mock_buffer_stream, has_submitted_buffer())
        .WillByDefault(Invoke([&stream1_visible] { return stream1_visible; }));
    ON_CALL(*mock_buffer_stream1, has_submitted_buffer())
        .WillByDefault(Invoke([&stream2_visible] { return stream2_visible; }));
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {} },
        { mock_buffer_stream1, displacement, {} },
    };
    surface.set_streams(streams);

    auto renderables = surface.generate_renderables(this);
    EXPECT_FALSE(surface.visible());
    EXPECT_THAT(renderables.size(), Eq(0));

    stream2_visible = true;
    renderables = surface.generate_renderables(this);
    EXPECT_TRUE(surface.visible());
    EXPECT_THAT(renderables.size(), Eq(1));

    stream1_visible = true;
    renderables = surface.generate_renderables(this);
    EXPECT_TRUE(surface.visible());
    EXPECT_THAT(renderables.size(), Eq(2));
}

TEST_F(BasicSurfaceTest, buffers_ready_correctly_reported)
{
    using namespace testing;
    auto buffer_stream0 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    EXPECT_CALL(*mock_buffer_stream, buffers_ready_for_compositor(_))
        .WillOnce(Return(0))
        .WillOnce(Return(0))
        .WillOnce(Return(2));
    EXPECT_CALL(*buffer_stream0, buffers_ready_for_compositor(_))
        .WillOnce(Return(1))
        .WillOnce(Return(0))
        .WillOnce(Return(1));
    EXPECT_CALL(*buffer_stream1, buffers_ready_for_compositor(_))
        .WillOnce(Return(3))
        .WillOnce(Return(0))
        .WillOnce(Return(1));

    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0}, {} },
        { buffer_stream0, {0,0}, {} },
        { buffer_stream1, {0,0}, {} },
    };
    surface.set_streams(streams);
    EXPECT_THAT(surface.buffers_ready_for_compositor(this), Eq(3));
    EXPECT_THAT(surface.buffers_ready_for_compositor(this), Eq(0));
    EXPECT_THAT(surface.buffers_ready_for_compositor(this), Eq(2));
}

TEST_F(BasicSurfaceTest, buffer_streams_produce_correctly_sized_renderables)
{
    using namespace testing;
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    geom::Displacement d0{0,0};
    geom::Displacement d1{19,99};
    geom::Size size0 {100, 101};
    geom::Size size1 {102, 103};
    ON_CALL(*mock_buffer_stream, stream_size())
        .WillByDefault(Return(size0));
    ON_CALL(*buffer_stream, stream_size())
        .WillByDefault(Return(size1));

    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, d0, {} },
        { buffer_stream, d1, {} },
    };
    surface.set_streams(streams);

    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(2));
    EXPECT_THAT(renderables[0], IsRenderableOfSize(size0));
    EXPECT_THAT(renderables[1], IsRenderableOfSize(size1));
}

TEST_F(BasicSurfaceTest, renderables_of_transparent_buffer_streams_are_shaped)
{
    using namespace testing;
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    geom::Displacement d0{0,0};
    geom::Displacement d1{19,99};
    ON_CALL(*buffer_stream, pixel_format())
        .WillByDefault(Return(mir_pixel_format_argb_8888));

    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, d0, {} },
        { buffer_stream, d1, {} },
    };
    surface.set_streams(streams);

    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(2));
    EXPECT_THAT(renderables[0]->shaped(), false);
    EXPECT_THAT(renderables[1]->shaped(), true);
}

namespace
{
struct VisibilityObserver : ms::NullSurfaceObserver
{
    void attrib_changed(mir::scene::Surface const*, MirWindowAttrib attrib, int value) override
    {
        if (attrib == mir_window_attrib_visibility)
        {
            if (value == mir_window_visibility_occluded)
                hides_++;
            else if (value == mir_window_visibility_exposed)
                exposes_++;
        }
    }
    unsigned int exposes()
    {
        return exposes_;
    }
    unsigned int hides()
    {
        return hides_;
    }
private:
    unsigned int exposes_{0};
    unsigned int hides_{0};
};
}

TEST_F(BasicSurfaceTest, notifies_when_first_visible)
{
    using namespace testing;
    auto observer = std::make_shared<VisibilityObserver>();
    surface.add_observer(observer);

    EXPECT_THAT(observer->exposes(), Eq(0));
    EXPECT_THAT(observer->hides(), Eq(0));
    surface.configure(mir_window_attrib_visibility, mir_window_visibility_exposed);

    EXPECT_THAT(observer->exposes(), Eq(1));
    EXPECT_THAT(observer->hides(), Eq(0));
}
