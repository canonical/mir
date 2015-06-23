/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "mir/test/doubles/mock_buffer_stream.h"
#include "mir/test/doubles/mock_input_sender.h"
#include "mir/test/doubles/stub_input_sender.h"
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
    MOCK_METHOD2(attrib_changed, void(MirSurfaceAttrib, int));
    MOCK_METHOD1(hidden_set_to, void(bool));
    MOCK_METHOD1(renamed, void(char const*));
    MOCK_METHOD0(client_surface_close_requested, void());
};

void post_a_frame(ms::BasicSurface& surface)
{
    /*
     * Make sure there's a frame ready. Otherwise visible()==false and the
     * input_area will never report it containing anything for all the tests
     * that use it.
     */
    mtd::StubBuffer buffer;
    surface.primary_buffer_stream()->swap_buffers(&buffer, [&](mir::graphics::Buffer*){});
}

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
    std::shared_ptr<mi::InputSender> const stub_input_sender = std::make_shared<mtd::StubInputSender>();
    testing::NiceMock<mtd::MockInputSender> mock_sender;

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
        std::shared_ptr<mg::CursorImage>(),
        report};

    BasicSurfaceTest()
    {
        post_a_frame(surface);
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

TEST_F(BasicSurfaceTest, primary_buffer_stream)
{
    EXPECT_THAT(surface.primary_buffer_stream(), Eq(mock_buffer_stream));
}

TEST_F(BasicSurfaceTest, buffer_stream_ids_always_unique)
{
    int const n = 10;
    std::array<std::unique_ptr<ms::BasicSurface>, n> surfaces;

    std::multiset<mg::Renderable::ID> ids;
    for (auto& surface : surfaces)
    {
        surface = std::make_unique<ms::BasicSurface>(
                name, rect, false, std::make_shared<testing::NiceMock<mtd::MockBufferStream>>(),
                std::shared_ptr<mi::InputChannel>(), stub_input_sender,
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
                name, rect, false, mock_buffer_stream,
                std::shared_ptr<mi::InputChannel>(), stub_input_sender,
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
    post_a_frame(surface);

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
    post_a_frame(surface);

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
    post_a_frame(surface);

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

TEST_F(BasicSurfaceTest, test_surface_set_alpha_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    surface.add_observer(observer);
    post_a_frame(surface);

    float alpha = 0.5f;
    surface.set_alpha(0.5f);
    EXPECT_THAT(alpha, FloatEq(surface.alpha()));
}

TEST_F(BasicSurfaceTest, test_surface_is_opaque_by_default)
{
    using namespace testing;

    EXPECT_THAT(1.0f, FloatEq(surface.alpha()));
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
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
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
    post_a_frame(surface);

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
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
        std::shared_ptr<mg::CursorImage>(),
        report};

    surface.add_observer(observer);
    post_a_frame(surface);

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
    EXPECT_CALL(*mock_buffer_stream, has_submitted_buffer())
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));

    ms::BasicSurface surface{
        name,
        geom::Rectangle{{0,0}, {100,100}},
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
        std::shared_ptr<mg::CursorImage>(),
        report};

    EXPECT_FALSE(surface.input_area_contains({50,50}));
    EXPECT_TRUE(surface.input_area_contains({50,50}));
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
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
        std::shared_ptr<mg::CursorImage>(),
        report};

    EXPECT_EQ(child.parent(), parent);
}

namespace
{

struct AttributeTestParameters
{
    MirSurfaceAttrib attribute;
    int default_value;
    int a_valid_value;
    int an_invalid_value;
};

struct BasicSurfaceAttributeTest : public BasicSurfaceTest,
    public ::testing::WithParamInterface<AttributeTestParameters>
{
};

AttributeTestParameters const surface_visibility_test_parameters{
    mir_surface_attrib_visibility,
    mir_surface_visibility_exposed,
    mir_surface_visibility_occluded,
    -1
};

AttributeTestParameters const surface_type_test_parameters{
    mir_surface_attrib_type,
    mir_surface_type_normal,
    mir_surface_type_freestyle,
    -1
};

AttributeTestParameters const surface_state_test_parameters{
    mir_surface_attrib_state,
    mir_surface_state_restored,
    mir_surface_state_fullscreen,
    1178312
};

AttributeTestParameters const surface_swapinterval_test_parameters{
    mir_surface_attrib_swapinterval,
    1,
    0,
    -1
};

AttributeTestParameters const surface_dpi_test_parameters{
    mir_surface_attrib_dpi,
    0,
    90,
    -1
};

AttributeTestParameters const surface_focus_test_parameters{
    mir_surface_attrib_focus,
    mir_surface_unfocused,
    mir_surface_focused,
    -1
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
    EXPECT_CALL(mock_surface_observer, attrib_changed(attribute, value1))
        .Times(1);
    EXPECT_CALL(mock_surface_observer, attrib_changed(attribute, value2))
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

    EXPECT_CALL(mock_surface_observer, attrib_changed(attribute, another_value))
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
    auto const& invalid_value = params.an_invalid_value;
    
    EXPECT_THROW({
            surface.configure(attribute, invalid_value);
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

TEST_F(BasicSurfaceTest, calls_send_event_on_consume)
{
    using namespace ::testing;

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        mt::fake_shared(mock_sender),
        nullptr,
        report};

    EXPECT_CALL(mock_sender, send_event(_,_));

    surface.consume(*mev::make_event(mir_prompt_session_state_started));
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
    EXPECT_CALL(observer1, hidden_set_to(true)).Times(2);
    EXPECT_CALL(observer3, hidden_set_to(true)).Times(2);

    auto const remove_observer = [&]{
        surface.remove_observer(mt::fake_shared(observer2));
    };

    EXPECT_CALL(observer2, hidden_set_to(true)).Times(1)
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

    EXPECT_CALL(mock_surface_observer, client_surface_close_requested()).Times(1);

    surface.add_observer(mt::fake_shared(mock_surface_observer));

    surface.request_client_surface_close();
}

TEST_F(BasicSurfaceTest, notifies_of_rename)
{
    using namespace testing;

    MockSurfaceObserver mock_surface_observer;
    surface.add_observer(mt::fake_shared(mock_surface_observer));

    EXPECT_CALL(mock_surface_observer, renamed(StrEq("Steve")));

    surface.rename("Steve");
}

MATCHER_P(IsRenderableOfPosition, pos, "is renderable with position")
{
    return (pos == arg->screen_position().top_left);
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
        { mock_buffer_stream, {0,0}},
        { buffer_stream0, d0 },
        { buffer_stream1, d1 },
        { buffer_stream2, d2 }
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
        { mock_buffer_stream, {0,0}},
        { buffer_stream, d }
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

//TODO: (kdub) This should be a temporary behavior while the buffer stream the surface was created
//with is still more important than the rest of the streams. One will soon be able to 
//remove the created-with bufferstream.
TEST_F(BasicSurfaceTest, cannot_remove_primary_buffer_stream_for_now)
{
    using namespace testing;
    geom::Displacement d0{19,99};
    geom::Displacement d1{21,101};
    geom::Displacement d2{20,9};
    auto buffer_stream0 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream1 = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto buffer_stream2 = std::make_shared<NiceMock<mtd::MockBufferStream>>();

    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0} },
    };
    surface.set_streams(streams);
    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(1));
    EXPECT_THAT(renderables[0], IsRenderableOfPosition(rect.top_left));

    EXPECT_THROW({
        surface.set_streams({});
    }, std::logic_error);
}

TEST_F(BasicSurfaceTest, showing_brings_all_streams_up_to_date)
{
    using namespace testing;
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0} },
        { buffer_stream, {0,0} }
    };
    surface.set_streams(streams);

    EXPECT_CALL(*buffer_stream, drop_old_buffers()).Times(Exactly(1));
    EXPECT_CALL(*mock_buffer_stream, drop_old_buffers()).Times(Exactly(1));

    surface.configure(mir_surface_attrib_visibility, mir_surface_visibility_occluded);
    surface.configure(mir_surface_attrib_visibility, mir_surface_visibility_exposed);
    surface.configure(mir_surface_attrib_visibility, mir_surface_visibility_exposed);
}

//TODO: per-stream alpha and swapinterval seems useful
TEST_F(BasicSurfaceTest, changing_alpha_effects_all_streams)
{
    using namespace testing;
    auto alpha = 0.3;
    
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0} },
        { buffer_stream, {0,0} }
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

TEST_F(BasicSurfaceTest, changing_inverval_effects_all_streams)
{
    using namespace testing;
    
    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams = {
        { mock_buffer_stream, {0,0} },
        { buffer_stream, {0,0} }
    };

    EXPECT_CALL(*mock_buffer_stream, allow_framedropping(true));
    EXPECT_CALL(*buffer_stream, allow_framedropping(true));

    surface.set_streams(streams);
    surface.configure(mir_surface_attrib_swapinterval, 0);
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
        { mock_buffer_stream, {0,0} },
        { mock_buffer_stream1, displacement },
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
        { mock_buffer_stream, {0,0}},
        { buffer_stream0, {0,0} },
        { buffer_stream1, {0,0} },
    };
    surface.set_streams(streams);
    EXPECT_THAT(surface.buffers_ready_for_compositor(this), Eq(3));
    EXPECT_THAT(surface.buffers_ready_for_compositor(this), Eq(0));
    EXPECT_THAT(surface.buffers_ready_for_compositor(this), Eq(2));
}
