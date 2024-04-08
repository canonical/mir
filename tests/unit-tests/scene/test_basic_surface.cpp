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


#include "src/server/scene/basic_surface.h"
#include "src/server/scene/surface_change_notification.h"

#include "mir/events/event_private.h"
#include "mir/frontend/event_sink.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/displacement.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/events/event_builders.h"

#include "mir/test/doubles/fake_display_configuration_observer_registrar.h"
#include "mir/test/doubles/stub_cursor_image.h"
#include "mir/test/doubles/mock_buffer_stream.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/geometry_matchers.h"

#include "src/server/report/null_report_factory.h"

#include <algorithm>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mw = mir::wayland;
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
    MOCK_METHOD(void, attrib_changed, (ms::Surface const*, MirWindowAttrib, int), (override));
    MOCK_METHOD(void, window_resized_to, (ms::Surface const*, geom::Size const&), (override));
    MOCK_METHOD(void, content_resized_to, (ms::Surface const*, geom::Size const&), (override));
    MOCK_METHOD(void, frame_posted, (ms::Surface const*, geom::Rectangle const&), (override));
    MOCK_METHOD(void, hidden_set_to, (ms::Surface const*, bool), (override));
    MOCK_METHOD(void, renamed, (ms::Surface const*, std::string const&), (override));
    MOCK_METHOD(void, client_surface_close_requested, (ms::Surface const*), (override));
    MOCK_METHOD(void, cursor_image_set_to, (ms::Surface const*, std::weak_ptr<mir::graphics::CursorImage> const&), (override));
    MOCK_METHOD(void, cursor_image_removed, (ms::Surface const*), (override));
    MOCK_METHOD(void, application_id_set_to, (ms::Surface const*, std::string const&), (override));
    MOCK_METHOD(void, entered_output, (ms::Surface const*, mg::DisplayConfigurationOutputId const&), (override));
    MOCK_METHOD(void, left_output, (ms::Surface const*, mg::DisplayConfigurationOutputId const&), (override));
};

struct BasicSurfaceTest : public testing::Test
{
    std::string const name{"aa"};
    geom::Rectangle const rect{{4,7},{12,15}};

    testing::NiceMock<MockCallback> mock_callback;
    std::function<void()> null_change_cb{[]{}};
    std::function<void()> mock_change_cb{std::bind(&MockCallback::call, &mock_callback)};
    std::shared_ptr<testing::NiceMock<mtd::MockBufferStream>> mock_buffer_stream =
        std::make_shared<testing::NiceMock<mtd::MockBufferStream>>();
    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    void const* compositor_id{nullptr};
    std::list<ms::StreamInfo> streams { { mock_buffer_stream, {}, {} } };
    std::shared_ptr<testing::NiceMock<MockSurfaceObserver>> mock_surface_observer =
        std::make_shared<testing::NiceMock<MockSurfaceObserver>>();
    mtd::ExplicitExecutor executor;
    std::shared_ptr<mtd::FakeDisplayConfigurationObserverRegistrar> display_config_registrar =
        std::make_shared<mtd::FakeDisplayConfigurationObserverRegistrar>();

    ms::BasicSurface surface{
        nullptr /* session */,
        {} /* wayland_surface */,
        name,
        rect,
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};

    std::shared_ptr<ms::SurfaceChangeNotification> observer =
        std::make_shared<ms::SurfaceChangeNotification>(
            &surface,
            mock_change_cb,
            [this](geom::Rectangle const&){mock_change_cb();});

    BasicSurfaceTest()
    {
        // use an opaque pixel format by default
        ON_CALL(*mock_buffer_stream, pixel_format())
            .WillByDefault(testing::Return(mir_pixel_format_xrgb_8888));
    }

    void TearDown() override
    {
        executor.execute();
    }
};

}

TEST_F(BasicSurfaceTest, basics)
{
    EXPECT_EQ(name, surface.name());
    EXPECT_EQ(rect.size, surface.window_size());
    EXPECT_EQ(rect.size, surface.content_size());
    EXPECT_EQ(rect.top_left, surface.top_left());
    for (auto& renderable : surface.generate_renderables(this))
        EXPECT_FALSE(renderable->shaped());
}

TEST_F(BasicSurfaceTest, can_be_created_with_session)
{
    using namespace testing;

    auto const session = std::make_shared<mtd::StubSession>();
    ms::BasicSurface surface{
        session,
        {} /* wayland_surface */,
        name,
        geom::Rectangle{{0,0}, {100,100}},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};

    EXPECT_THAT(surface.session().lock(), Eq(session));
}

TEST_F(BasicSurfaceTest, buffer_stream_ids_always_unique)
{
    int const n = 10;
    std::array<std::unique_ptr<ms::BasicSurface>, n> surfaces;

    std::multiset<mg::Renderable::ID> ids;
    for (auto& surface : surfaces)
    {
        surface = std::make_unique<ms::BasicSurface>(
                nullptr /* session */,
                mw::Weak<mf::WlSurface>{},
                name,
                rect,
                mir_pointer_unconfined,
                std::list<ms::StreamInfo> {
                    { std::make_shared<testing::NiceMock<mtd::MockBufferStream>>(), {}, {} } },
                std::shared_ptr<mg::CursorImage>(),
                report,
                display_config_registrar);
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
                nullptr /* session */,
                mw::Weak<mf::WlSurface>{},
                name,
                rect,
                mir_pointer_unconfined,
                streams,
                std::shared_ptr<mg::CursorImage>(),
                report,
                display_config_registrar);

        for (auto& renderable : surface->generate_renderables(this))
            EXPECT_THAT(renderable->id(), testing::Ne(nullptr));
    }
}

TEST_F(BasicSurfaceTest, update_top_left)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    surface.register_interest(observer, executor);

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

    surface.register_interest(observer, executor);

    EXPECT_EQ(rect.size, surface.window_size());
    EXPECT_NE(new_size, surface.window_size());

    auto renderables = surface.generate_renderables(compositor_id);
    ASSERT_THAT(renderables.size(), testing::Eq(1));
    auto old_transformation = renderables[0]->transformation();

    surface.resize(new_size);
    EXPECT_EQ(new_size, surface.window_size());
    // Size no longer affects transformation:
    renderables = surface.generate_renderables(compositor_id);
    ASSERT_THAT(renderables.size(), testing::Eq(1));
    EXPECT_THAT(renderables[0]->transformation(), testing::Eq(old_transformation));
}

TEST_F(BasicSurfaceTest, observer_notified_of_window_resize)
{
    using namespace testing;

    geom::Size const new_size{34, 56};

    ASSERT_THAT(new_size, Ne(surface.window_size())) << "Precondition failed";

    EXPECT_CALL(*mock_surface_observer, window_resized_to(_, new_size))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.resize(new_size);
}

TEST_F(BasicSurfaceTest, observer_notified_of_content_resize)
{
    using namespace testing;

    geom::Size const new_size{34, 56};

    ASSERT_THAT(new_size, Ne(surface.content_size())) << "Precondition failed";

    EXPECT_CALL(*mock_surface_observer, content_resized_to(_, new_size))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.resize(new_size);
}

TEST_F(BasicSurfaceTest, resize_notifications_only_sent_when_size_changed)
{
    using namespace testing;

    geom::Size const new_size{34, 56};

    ASSERT_THAT(rect.size, Eq(surface.window_size())) << "Precondition failed";
    ASSERT_THAT(rect.size, Eq(surface.content_size())) << "Precondition failed";
    ASSERT_THAT(new_size, Ne(surface.content_size())) << "Precondition failed";

    EXPECT_CALL(*mock_surface_observer, window_resized_to(_, _))
        .Times(1);
    EXPECT_CALL(*mock_surface_observer, content_resized_to(_, _))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.resize(rect.size);
    surface.resize(new_size);
    surface.resize(new_size);
}

TEST_F(BasicSurfaceTest, only_content_is_notified_of_resize_when_frame_geometry_set)
{
    using namespace testing;

    geom::DeltaY const top{3};
    geom::DeltaX const left{2};
    geom::DeltaY const bottom{5};
    geom::DeltaX const right{1};
    geom::Size const new_content_size{
        rect.size.width - left - right,
        rect.size.height - top - bottom};

    EXPECT_CALL(*mock_surface_observer, window_resized_to(_, _))
        .Times(0);
    EXPECT_CALL(*mock_surface_observer, content_resized_to(_, new_content_size))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.set_window_margins(top, left, bottom, right);
}

TEST_F(BasicSurfaceTest, window_size_equals_content_size_when_frame_geometry_is_not_set)
{
    using namespace testing;

    geom::Size const new_size{34, 56};

    EXPECT_THAT(rect.size, Eq(surface.window_size()));
    EXPECT_THAT(rect.size, Eq(surface.content_size()));
    EXPECT_THAT(new_size, Ne(surface.window_size()));
    EXPECT_THAT(new_size, Ne(surface.content_size()));

    surface.resize(new_size);
    EXPECT_THAT(new_size, Eq(surface.window_size()));
    EXPECT_THAT(new_size, Eq(surface.content_size()));
}

TEST_F(BasicSurfaceTest, window_size_differs_from_content_size_when_frame_geometry_is_set)
{
    using namespace testing;

    geom::DeltaY const top{3};
    geom::DeltaX const left{2};
    geom::DeltaY const bottom{5};
    geom::DeltaX const right{1};
    geom::Size const new_window_size{34, 56};
    geom::Size const new_content_size{
        new_window_size.width - left - right,
        new_window_size.height - top - bottom};

    EXPECT_THAT(rect.size, Eq(surface.window_size()));
    EXPECT_THAT(rect.size, Eq(surface.content_size()));
    EXPECT_THAT(new_window_size, Ne(surface.window_size()));
    EXPECT_THAT(new_content_size, Ne(surface.content_size()));

    surface.set_window_margins(top, left, bottom, right);
    surface.resize(new_window_size);
    EXPECT_THAT(new_content_size, Ne(surface.window_size())) << "window_size is what content_size should be";
    EXPECT_THAT(new_window_size, Ne(surface.content_size())) << "content_size is what window_size should be";
    EXPECT_THAT(new_window_size, Eq(surface.window_size()));
    EXPECT_THAT(new_content_size, Eq(surface.content_size()));
}

TEST_F(BasicSurfaceTest, only_content_is_resized_when_frame_geometry_set)
{
    using namespace testing;

    geom::DeltaY const top{3};
    geom::DeltaX const left{2};
    geom::DeltaY const bottom{5};
    geom::DeltaX const right{1};
    geom::Size const new_content_size{
        rect.size.width - left - right,
        rect.size.height - top - bottom};

    surface.set_window_margins(top, left, bottom, right);

    ASSERT_THAT(rect.size, Eq(surface.window_size()));
    ASSERT_THAT(new_content_size, Eq(surface.content_size()));
}

TEST_F(BasicSurfaceTest, input_bounds_size_equals_content_size)
{
    using namespace testing;

    geom::DeltaY const top{3};
    geom::DeltaX const left{2};
    geom::DeltaY const bottom{5};
    geom::DeltaX const right{1};
    geom::Size const new_content_size{
        rect.size.width - left - right,
        rect.size.height - top - bottom};

    EXPECT_THAT(new_content_size, Ne(surface.input_bounds().size));

    surface.set_window_margins(top, left, bottom, right);
    EXPECT_THAT(surface.window_size(), Ne(surface.input_bounds().size)) << "input_bounds.size not affected by frame geom";
    EXPECT_THAT(new_content_size, Eq(surface.input_bounds().size));
}

TEST_F(BasicSurfaceTest, input_bounds_top_left_is_offset_by_frame_geom)
{
    using namespace testing;

    geom::DeltaY const top{3};
    geom::DeltaX const left{2};
    geom::DeltaY const bottom{5};
    geom::DeltaX const right{1};
    geom::Point const new_content_top_left{rect.top_left + geom::Displacement{left, top}};

    EXPECT_THAT(new_content_top_left, Ne(surface.input_bounds().top_left));

    surface.set_window_margins(top, left, bottom, right);
    EXPECT_THAT(rect.top_left, Ne(surface.input_bounds().top_left)) << "input_bounds.top_left not affected by frame geom";
    EXPECT_THAT(new_content_top_left, Eq(surface.input_bounds().top_left));
}

TEST_F(BasicSurfaceTest, test_surface_set_transformation_updates_transform)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    surface.register_interest(observer, executor);

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
        nullptr /* session */,
        {} /* wayland_surface */,
        name,
        rect,
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};

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

    surface.register_interest(observer, executor);

    surface.set_hidden(true);
}

// a 1x1 window at (1,1) will get events at (1,1)
TEST_F(BasicSurfaceTest, default_region_is_surface_rectangle)
{
    geom::Point pt(1,1);
    geom::Size one_by_one{geom::Width{1}, geom::Height{1}};
    ms::BasicSurface surface{
        nullptr /* session */,
        {} /* wayland_surface */,
        name,
        geom::Rectangle{pt, one_by_one},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};

    surface.register_interest(observer, executor);

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
        nullptr /* session */,
        {} /* wayland_surface */,
        name,
        geom::Rectangle{{0,0}, {100,100}},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};

    EXPECT_CALL(*mock_buffer_stream, has_submitted_buffer())
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));

    EXPECT_FALSE(surface.input_area_contains({50,50}));
    EXPECT_TRUE(surface.input_area_contains({50,50}));
}

TEST_F(BasicSurfaceTest, surface_doesnt_get_input_outside_clip_area)
{
    ms::BasicSurface surface{
        nullptr /* session */,
        {} /* wayland_surface */,
        name,
        geom::Rectangle{{0,0}, {100,100}},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};

    surface.set_clip_area(std::optional<geom::Rectangle>({{0,0}, {50,50}}));

    EXPECT_FALSE(surface.input_area_contains({75,75}));
    EXPECT_TRUE(surface.input_area_contains({25,25}));

    surface.set_clip_area(std::optional<geom::Rectangle>());

    EXPECT_TRUE(surface.input_area_contains({75,75}));
}

TEST_F(BasicSurfaceTest, set_input_region)
{
    std::vector<geom::Rectangle> const rectangles = {
        {{geom::X{0}, geom::Y{0}}, {geom::Width{1}, geom::Height{1}}}, //region0
        {{geom::X{1}, geom::Y{1}}, {geom::Width{1}, geom::Height{1}}}  //region1
    };

    surface.register_interest(observer, executor);

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
    geom::Rectangle const new_rect{rect.top_left,{20,20}};
    surface.resize(new_rect.size);

    ASSERT_GT(new_rect.size.width, rect.size.width) << "Precondition failed";
    ASSERT_GT(new_rect.size.height, rect.size.height) << "Precondition failed";

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

TEST_F(BasicSurfaceTest, adjusts_default_input_region_for_frame_geometry)
{
    geom::DeltaY const top{3};
    geom::DeltaX const left{2};
    geom::DeltaY const bottom{5};
    geom::DeltaX const right{1};
    geom::Point const content_top_left{rect.top_left + geom::Displacement{left, top}};
    geom::Size const content_size{
        rect.size.width - left - right,
        rect.size.height - top - bottom};
    geom::Rectangle const input_region{content_top_left, content_size};

    surface.set_window_margins(top, left, bottom, right);

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
            if (input_region.contains(test_pt))
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

TEST_F(BasicSurfaceTest, adjusts_explicit_input_region_for_frame_geometry)
{
    geom::DeltaY const top{3};
    geom::DeltaX const left{2};
    geom::DeltaY const bottom{5};
    geom::DeltaX const right{1};
    geom::Rectangle const local_input_region{{3, 2}, {4, 5}};
    geom::Point const global_input_region_top_left{
        rect.top_left +
        geom::Displacement{left, top} +
        as_displacement(local_input_region.top_left)};
    geom::Rectangle const global_input_region{
        global_input_region_top_left,
        local_input_region.size};

    surface.set_window_margins(top, left, bottom, right);
    surface.set_input_region({local_input_region});

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
            if (global_input_region.contains(test_pt))
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
        nullptr /* session */,
        {} /* wayland_surface */,
        name,
        geom::Rectangle{{0,0}, {100,100}},
        parent,
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};

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
    mir_window_focus_state_active,
    mir_window_focus_state_active + 1,
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

    InSequence s;
    EXPECT_CALL(*mock_surface_observer, attrib_changed(_, attribute, value1))
        .Times(1);
    EXPECT_CALL(*mock_surface_observer, attrib_changed(_, attribute, value2))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);

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

    EXPECT_CALL(*mock_surface_observer, attrib_changed(_, attribute, another_value))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);

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

INSTANTIATE_TEST_SUITE_P(SurfaceTypeAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_type_test_parameters));

INSTANTIATE_TEST_SUITE_P(SurfaceVisibilityAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_visibility_test_parameters));

INSTANTIATE_TEST_SUITE_P(SurfaceStateAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_state_test_parameters));

INSTANTIATE_TEST_SUITE_P(SurfaceDPIAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_dpi_test_parameters));

INSTANTIATE_TEST_SUITE_P(SurfaceFocusAttributeTest, BasicSurfaceAttributeTest,
   ::testing::Values(surface_focus_test_parameters));

TEST_F(BasicSurfaceTest, default_focus_state)
{
    EXPECT_EQ(mir_window_focus_state_unfocused, surface.focus_state());
}

TEST_F(BasicSurfaceTest, focus_state_can_be_set)
{
    surface.set_focus_state(mir_window_focus_state_focused);
    EXPECT_EQ(mir_window_focus_state_focused, surface.focus_state());
}

TEST_F(BasicSurfaceTest, notifies_about_focus_state_changes)
{
    using namespace testing;

    EXPECT_CALL(*mock_surface_observer, attrib_changed(_, mir_window_attrib_focus, mir_window_focus_state_focused))
        .Times(1);
    EXPECT_CALL(*mock_surface_observer, attrib_changed(_, mir_window_attrib_focus, mir_window_focus_state_unfocused))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);

    surface.set_focus_state(mir_window_focus_state_focused);
    surface.set_focus_state(mir_window_focus_state_unfocused);
}

TEST_F(BasicSurfaceTest, does_not_notify_if_focus_state_is_unchanged)
{
    using namespace testing;

    EXPECT_CALL(*mock_surface_observer, attrib_changed(_, mir_window_attrib_focus, mir_window_focus_state_focused))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);

    surface.set_focus_state(mir_window_focus_state_unfocused); // will not notify, since it is default
    surface.set_focus_state(mir_window_focus_state_focused);
    surface.set_focus_state(mir_window_focus_state_focused);
}

TEST_F(BasicSurfaceTest, throws_on_invalid_focus_state)
{
    using namespace testing;

    auto const invalid_state = static_cast<MirWindowFocusState>(mir_window_focus_state_active + 1);

    EXPECT_THROW({
            surface.configure(mir_window_attrib_focus, invalid_state);
        }, std::logic_error);
}

TEST_F(BasicSurfaceTest, notifies_about_cursor_image_change)
{
    using namespace testing;

    auto cursor_image = std::make_shared<mtd::StubCursorImage>();
    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _));

    surface.register_interest(mock_surface_observer, executor);
    surface.set_cursor_image(cursor_image);
}

TEST_F(BasicSurfaceTest, notifies_about_cursor_image_removal)
{
    using namespace testing;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _)).Times(0);
    EXPECT_CALL(*mock_surface_observer, cursor_image_removed(_));

    surface.register_interest(mock_surface_observer, executor);
    surface.set_cursor_image({});
}

TEST_F(BasicSurfaceTest, when_frame_is_posted_an_observer_is_notified_of_frame_with_correct_size)
{
    using namespace testing;

    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();

    surface.resize({100, 150}); // should not affect frame_posted notification
    surface.register_interest(mock_surface_observer, executor);
    surface.set_streams({ms::StreamInfo{buffer_stream, {}, {}}});
    ON_CALL(*buffer_stream, stream_size())
        .WillByDefault(Return(rect.size));

    EXPECT_CALL(*mock_surface_observer, frame_posted(_, mt::RectSizeEq(rect.size)));
    buffer_stream->frame_posted_callback(rect.size);
}

TEST_F(BasicSurfaceTest, when_stream_size_differs_from_buffer_size_an_observer_is_notified_of_frame_with_stream_size)
{
    using namespace testing;
    geom::Size const stream_size{rect.size * 1.5};

    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::function<void(geom::Size const&)> frame_posted_callback;
    ON_CALL(*buffer_stream, stream_size())
        .WillByDefault(Return(stream_size));

    surface.register_interest(mock_surface_observer, executor);
    surface.set_streams({ms::StreamInfo{buffer_stream, {}, {}}});

    EXPECT_CALL(*mock_surface_observer, frame_posted(_, mt::RectSizeEq(stream_size)));
    buffer_stream->frame_posted_callback(stream_size * 2);
}

TEST_F(BasicSurfaceTest, when_stream_info_has_explicit_size_an_observer_is_notified_of_frame_with_stream_info_size)
{
    using namespace testing;
    geom::Size const stream_info_size{rect.size * 1.5};
    geom::Size const stream_size{stream_info_size * 2};

    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::function<void(geom::Size const&)> frame_posted_callback;
    ON_CALL(*buffer_stream, stream_size())
        .WillByDefault(Return(stream_size));

    surface.register_interest(mock_surface_observer, executor);
    surface.set_streams({ms::StreamInfo{buffer_stream, {}, stream_info_size}});

    EXPECT_CALL(*mock_surface_observer, frame_posted(_, mt::RectSizeEq(stream_info_size)));
    buffer_stream->frame_posted_callback(stream_size);
}

TEST_F(BasicSurfaceTest, when_frame_is_posted_an_observer_is_notified_of_frame_at_origin)
{
    using namespace testing;

    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    surface.register_interest(mock_surface_observer, executor);
    surface.set_streams({ms::StreamInfo{buffer_stream, {}, {}}});

    EXPECT_CALL(*mock_surface_observer, frame_posted(_, mt::RectTopLeftEq(geom::Point{})));
    buffer_stream->frame_posted_callback(rect.size);
}

TEST_F(BasicSurfaceTest, when_stream_info_has_offset_an_observer_is_notified_of_frame_with_correct_offset)
{
    using namespace testing;
    geom::Displacement const stream_info_offset{7, 10};

    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::function<void(geom::Size const&)> frame_posted_callback;

    surface.register_interest(mock_surface_observer, executor);
    surface.set_streams({ms::StreamInfo{buffer_stream, stream_info_offset, {}}});

    EXPECT_CALL(*mock_surface_observer, frame_posted(_, mt::RectTopLeftEq(geom::Point{} + stream_info_offset)));
    buffer_stream->frame_posted_callback(rect.size);
}

TEST_F(BasicSurfaceTest, when_surface_has_margins_an_observer_is_notified_of_frame_with_correct_offset)
{
    using namespace testing;
    geom::DeltaY const margin_top{4}, margin_bottom{6};
    geom::DeltaX const margin_left{3}, margin_right{5};

    auto buffer_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::function<void(geom::Size const&)> frame_posted_callback;

    surface.register_interest(mock_surface_observer, executor);
    surface.set_streams({ms::StreamInfo{buffer_stream, {}, {}}});

    EXPECT_CALL(*mock_surface_observer, frame_posted(_, mt::RectTopLeftEq(geom::Point{} + margin_top + margin_left)));
    surface.set_window_margins(margin_top, margin_left, margin_bottom, margin_right);
    buffer_stream->frame_posted_callback({20, 30});
}

TEST_F(BasicSurfaceTest, default_application_id)
{
    EXPECT_EQ("", surface.application_id());
}

TEST_F(BasicSurfaceTest, application_id_can_be_set)
{
    std::string const id{"test.id"};
    surface.set_application_id(id);
    EXPECT_EQ(id, surface.application_id());
}

TEST_F(BasicSurfaceTest, notifies_about_application_id_changes)
{
    using namespace testing;

    std::string const id{"test.id"};

    EXPECT_CALL(*mock_surface_observer, application_id_set_to(_, id))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);

    surface.set_application_id(id);
}

TEST_F(BasicSurfaceTest, does_not_notify_if_application_id_is_unchanged)
{
    using namespace testing;

    std::string const id{"test.id"};

    EXPECT_CALL(*mock_surface_observer, application_id_set_to(_, id))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);

    surface.set_application_id("");
    surface.set_application_id(id);
    surface.set_application_id(id);
}

TEST_F(BasicSurfaceTest, notifies_of_entered_output_upon_creation)
{
    using namespace testing;

    mir::graphics::DisplayConfigurationOutputId const id1{1};
    mir::graphics::DisplayConfigurationOutputId const id2{2};

    EXPECT_CALL(*mock_surface_observer, entered_output(_, id1))
        .Times(1);
    EXPECT_CALL(*mock_surface_observer, entered_output(_, id2))
        .Times(0);

    surface.register_interest(mock_surface_observer, executor);
    surface.move_to({0, 0});
}

TEST_F(BasicSurfaceTest, notifies_of_entered_output_on_move)
{
    using namespace testing;

    mir::graphics::DisplayConfigurationOutputId const id1{1};
    mir::graphics::DisplayConfigurationOutputId const id2{2};

    EXPECT_CALL(*mock_surface_observer, entered_output(_, id1))
        .Times(1);
    EXPECT_CALL(*mock_surface_observer, entered_output(_, id2))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.move_to({0, 0});
    surface.move_to({110, 0});
}

TEST_F(BasicSurfaceTest, notifies_of_left_output_on_move)
{
    using namespace testing;

    mir::graphics::DisplayConfigurationOutputId const id{1};

    EXPECT_CALL(*mock_surface_observer, left_output(_, id))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.move_to({0, 0});
    surface.move_to({110, 0});
}

TEST_F(BasicSurfaceTest, notifies_of_entered_output_when_resizing)
{
    using namespace testing;

    mir::graphics::DisplayConfigurationOutputId const id1{1};
    mir::graphics::DisplayConfigurationOutputId const id2{2};

    EXPECT_CALL(*mock_surface_observer, entered_output(_, id1))
        .Times(1);
    EXPECT_CALL(*mock_surface_observer, entered_output(_, id2))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.move_to({0, 0});
    surface.resize({150, 50});
}

TEST_F(BasicSurfaceTest, notifies_of_left_output_when_resizing)
{
    using namespace testing;

    mir::graphics::DisplayConfigurationOutputId const id{2};

    EXPECT_CALL(*mock_surface_observer, left_output(_, id))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.move_to({0, 0});
    surface.resize({150, 50});
    surface.resize({50, 50});
}

TEST_F(BasicSurfaceTest, notifies_of_left_output_when_output_is_disconnected)
{
    using namespace testing;

    mir::graphics::DisplayConfigurationOutputId const id{2};

    EXPECT_CALL(*mock_surface_observer, left_output(_, id))
        .Times(1);

    surface.register_interest(mock_surface_observer, executor);
    surface.move_to({0, 0});
    surface.resize({50, 50});
    surface.move_to({75, 0});
    display_config_registrar->disconnect_output(1);
}

TEST_F(BasicSurfaceTest, notifies_of_client_close_request)
{
    using namespace testing;

    EXPECT_CALL(*mock_surface_observer, client_surface_close_requested(_)).Times(1);

    surface.register_interest(mock_surface_observer, executor);

    surface.request_client_surface_close();
}

TEST_F(BasicSurfaceTest, notifies_of_rename)
{
    using namespace testing;

    surface.register_interest(mock_surface_observer, executor);

    EXPECT_CALL(*mock_surface_observer, renamed(_, StrEq("Steve")));

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
    surface.set_clip_area(std::optional<geom::Rectangle>({{200,0},{100,100}}));

    auto renderables = surface.generate_renderables(this);
    ASSERT_THAT(renderables.size(), Eq(0));

    surface.set_clip_area(std::optional<geom::Rectangle>({{0,0},{100,100}}));

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

    EXPECT_CALL(*buffer_stream0, set_frame_posted_callback(_))
        .Times(AtLeast(1));
    EXPECT_CALL(*buffer_stream1, set_frame_posted_callback(_))
        .Times(AtLeast(1));

    ms::BasicSurface child{
        nullptr /* session */,
        {} /* wayland_surface */,
        name,
        geom::Rectangle{{0,0}, {100,100}},
        std::weak_ptr<ms::Surface>{},
        mir_pointer_unconfined,
        streams,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar};
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

    EXPECT_CALL(*buffer_stream0, set_frame_posted_callback(_))
        .Times(AtLeast(1));
    EXPECT_CALL(*buffer_stream1, set_frame_posted_callback(_))
        .Times(AtLeast(1));

    surface.set_streams(streams);
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
    surface.register_interest(observer, executor);

    EXPECT_THAT(observer->exposes(), Eq(0));
    EXPECT_THAT(observer->hides(), Eq(0));
    surface.configure(mir_window_attrib_visibility, mir_window_visibility_exposed);

    executor.execute();

    EXPECT_THAT(observer->exposes(), Eq(1));
    EXPECT_THAT(observer->hides(), Eq(0));
}

TEST_F(BasicSurfaceTest, buffer_can_be_submitted_to_original_stream_after_surface_destroyed)
{
    using namespace testing;

    auto local_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> local_stream_list = { { local_stream, {}, {} } };
    std::function<void(geom::Size const&)> callback = [](auto){};

    EXPECT_CALL(*local_stream, set_frame_posted_callback(_))
        .Times(AtLeast(1))
        .WillRepeatedly(SaveArg<0>(&callback));

    auto surface = std::make_unique<ms::BasicSurface>(
        nullptr, // session
        mw::Weak<mf::WlSurface>{},
        name,
        rect,
        mir_pointer_unconfined,
        local_stream_list,
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar);

    surface.reset();
    callback({10, 10});
}

TEST_F(BasicSurfaceTest, buffer_can_be_submitted_to_set_stream_after_surface_destroyed)
{
    using namespace testing;

    auto local_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> local_stream_list = { { local_stream, {}, {} } };
    std::function<void(geom::Size const&)> callback = [](auto){};

    EXPECT_CALL(*local_stream, set_frame_posted_callback(_))
        .Times(AtLeast(1))
        .WillRepeatedly(SaveArg<0>(&callback));

    auto surface = std::make_unique<ms::BasicSurface>(
        nullptr, // session
        mw::Weak<mf::WlSurface>{},
        name,
        rect,
        mir_pointer_unconfined,
        streams, // use the default list from the class initially
        std::shared_ptr<mg::CursorImage>(),
        report,
        display_config_registrar);

    surface->set_streams(local_stream_list);

    surface.reset();
    callback({10, 10});
}
