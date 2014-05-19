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

#include "mir/frontend/event_sink.h"
#include "mir/geometry/rectangle.h"
#include "mir/scene/surface_configurator.h"

#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"

#include "src/server/report/null_report_factory.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mr = mir::report;
namespace ms = mir::scene;
namespace msh = mir::shell;
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

class StubEventSink : public mir::frontend::EventSink
{
public:
    void handle_event(MirEvent const&) override {}
    void handle_lifecycle_event(MirLifecycleState) override {}
    void handle_display_config_change(mir::graphics::DisplayConfiguration const&) override {}
};

struct StubSurfaceConfigurator : ms::SurfaceConfigurator
{
    int select_attribute_value(ms::Surface const&, MirSurfaceAttrib, int) override { return 0; }

    void attribute_set(ms::Surface const&, MirSurfaceAttrib, int) override { }
};

struct BasicSurfaceTest : public testing::Test
{
    void SetUp()
    {
        name = std::string("aa");
        top_left = geom::Point{geom::X{4}, geom::Y{7}};
        size = geom::Size{5, 9};
        rect = geom::Rectangle{top_left, size};
        null_change_cb = []{};
        mock_change_cb = std::bind(&MockCallback::call, &mock_callback);
    }
    std::string name;
    geom::Point top_left;
    geom::Size size;
    geom::Rectangle rect;

    testing::NiceMock<MockCallback> mock_callback;
    std::function<void()> null_change_cb;
    std::function<void()> mock_change_cb;
    std::shared_ptr<testing::NiceMock<mtd::MockBufferStream>> mock_buffer_stream =
        std::make_shared<testing::NiceMock<mtd::MockBufferStream>>();
    std::shared_ptr<StubSurfaceConfigurator> const stub_configurator = std::make_shared<StubSurfaceConfigurator>();
    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    void const* compositor_id{nullptr};
};

}

TEST_F(BasicSurfaceTest, basics)
{
    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    EXPECT_EQ(name, surface.name());
    EXPECT_EQ(rect.size, surface.size());
    EXPECT_EQ(rect.top_left, surface.top_left());
    EXPECT_FALSE(surface.compositor_snapshot(compositor_id)->shaped());
}

TEST_F(BasicSurfaceTest, id_always_unique)
{
    int const N = 10;
    std::unique_ptr<ms::BasicSurface> surfaces[N];

    for (int i = 0; i < N; ++i)
    {
        surfaces[i].reset(new ms::BasicSurface(
                name, rect, false, mock_buffer_stream,
                std::shared_ptr<mi::InputChannel>(), stub_configurator, report)
            );

        for (int j = 0; j < i; ++j)
        {
            ASSERT_NE(surfaces[j]->compositor_snapshot(compositor_id)->id(), surfaces[i]->compositor_snapshot(compositor_id)->id());
        }
    }
}

TEST_F(BasicSurfaceTest, id_never_invalid)
{
    int const N = 10;
    std::unique_ptr<ms::BasicSurface> surfaces[N];

    for (int i = 0; i < N; ++i)
    {
        surfaces[i].reset(new ms::BasicSurface(
                name, rect, false, mock_buffer_stream,
                std::shared_ptr<mi::InputChannel>(), stub_configurator, report)
            );

        ASSERT_TRUE(surfaces[i]->compositor_snapshot(compositor_id)->id());
    }
}

TEST_F(BasicSurfaceTest, update_top_left)
{
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
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

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
    surface.add_observer(observer);

    EXPECT_EQ(rect.size, surface.size());
    EXPECT_NE(new_size, surface.size());

    auto old_transformation = surface.compositor_snapshot(compositor_id)->transformation();

    surface.resize(new_size);
    EXPECT_EQ(new_size, surface.size());
    // Size no longer affects transformation:
    EXPECT_EQ(old_transformation, surface.compositor_snapshot(compositor_id)->transformation());
}

/*
 * Until logic is implemented to separate size() from client_size(), verify
 * they do return the same thing for backward compatibility.
 */
TEST_F(BasicSurfaceTest, size_equals_client_size)
{
    geom::Size const new_size{34, 56};

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

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

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
    surface.add_observer(observer);

    auto original_transformation = surface.compositor_snapshot(compositor_id)->transformation();
    glm::mat4 trans{0.1f, 0.5f, 0.9f, 1.3f,
                    0.2f, 0.6f, 1.0f, 1.4f,
                    0.3f, 0.7f, 1.1f, 1.5f,
                    0.4f, 0.8f, 1.2f, 1.6f};

    surface.set_transformation(trans);
    auto got = surface.compositor_snapshot(compositor_id)->transformation();
    EXPECT_NE(original_transformation, got);
    EXPECT_EQ(trans, got);
}

TEST_F(BasicSurfaceTest, test_surface_set_alpha_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
    surface.add_observer(observer);

    float alpha = 0.5f;
    surface.set_alpha(0.5f);
    EXPECT_THAT(alpha, FloatEq(surface.alpha()));
}

TEST_F(BasicSurfaceTest, test_surface_is_opaque_by_default)
{
    using namespace testing;
    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    EXPECT_THAT(1.0f, FloatEq(surface.alpha()));
    EXPECT_FALSE(surface.compositor_snapshot(compositor_id)->shaped());
}

TEST_F(BasicSurfaceTest, test_surface_visibility)
{
    using namespace testing;
    mtd::StubBuffer mock_buffer;
    EXPECT_CALL(*mock_buffer_stream, swap_client_buffers(_,_)).Times(2)
        .WillRepeatedly(InvokeArgument<1>(&mock_buffer));

    mir::graphics::Buffer* buffer = nullptr;
    auto const callback = [&](mir::graphics::Buffer* new_buffer) { buffer = new_buffer; };

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    //not visible by default
    EXPECT_FALSE(surface.visible());

    surface.set_hidden(false);
    //not renderable if no first frame has been posted by client, regardless of hide state
    EXPECT_FALSE(surface.visible());
    surface.set_hidden(true);
    EXPECT_FALSE(surface.visible());

    // The second call posts the buffer returned by first
    surface.swap_buffers(buffer, callback);
    surface.swap_buffers(buffer, callback);

    EXPECT_FALSE(surface.visible());

    surface.set_hidden(false);
    EXPECT_TRUE(surface.visible());
}

TEST_F(BasicSurfaceTest, test_surface_hidden_notifies_changes)
{
    using namespace testing;
    EXPECT_CALL(mock_callback, call())
        .Times(1);

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
    surface.add_observer(observer);

    surface.set_hidden(true);
}

TEST_F(BasicSurfaceTest, test_surface_frame_posted_notifies_changes)
{
    using namespace testing;
    mtd::StubBuffer mock_buffer;
    EXPECT_CALL(*mock_buffer_stream, swap_client_buffers(_,_)).Times(2)
        .WillRepeatedly(InvokeArgument<1>(&mock_buffer));

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
    surface.add_observer(observer);

    mir::graphics::Buffer* buffer = nullptr;
    auto const callback = [&](mir::graphics::Buffer* new_buffer) { buffer = new_buffer; };

    EXPECT_CALL(mock_callback, call()).Times(1);

    // The second call posts the buffer returned by first
    surface.swap_buffers(buffer, callback);
    surface.swap_buffers(buffer, callback);
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
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
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

TEST_F(BasicSurfaceTest, set_input_region)
{
    std::vector<geom::Rectangle> const rectangles = {
        {{geom::X{0}, geom::Y{0}}, {geom::Width{1}, geom::Height{1}}}, //region0
        {{geom::X{1}, geom::Y{1}}, {geom::Width{1}, geom::Height{1}}}  //region1
    };

    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    auto const observer = std::make_shared<ms::LegacySurfaceChangeNotification>(mock_change_cb);
    surface.add_observer(observer);

    surface.set_input_region(rectangles);

    std::vector<geom::Point> contained_pt
    {
        //region0 points
        geom::Point{geom::X{0}, geom::Y{0}},
        //region1 points
        geom::Point{geom::X{1}, geom::Y{1}},
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


TEST_F(BasicSurfaceTest, reception_mode_is_normal_by_default)
{
    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    EXPECT_EQ(mi::InputReceptionMode::normal, surface.reception_mode());
}

TEST_F(BasicSurfaceTest, reception_mode_can_be_changed)
{
    ms::BasicSurface surface{
        name,
        rect,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_configurator,
        report};

    surface.set_reception_mode(mi::InputReceptionMode::receives_all_input);

    EXPECT_EQ(mi::InputReceptionMode::receives_all_input, surface.reception_mode());
}
