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

#include "mir/events/event_builders.h"
#include "mir/scene/output_properties_cache.h"
#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"
#include "mir/scene/surface_event_source.h"

#include "mir/test/doubles/fake_display_configuration_observer_registrar.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/doubles/mock_buffer_stream.h"
#include "mir/test/doubles/mock_input_surface.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/event_matchers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mev = mir::events;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mr = mir::report;

namespace
{
struct SurfaceCreation : public ::testing::Test
{
    SurfaceCreation()
        : surface(
            nullptr /* session */,
            {} /* wayland_surface */,
            surface_name,
            rect,
            mir_pointer_unconfined,
            streams,
            nullptr /* cursor_image */,
            report,
            std::make_shared<mtd::FakeDisplayConfigurationObserverRegistrar>())
    {
    }

    void SetUp() override
    {
        using namespace testing;

        notification_count = 0;
        change_notification = [this]()
        {
            notification_count++;
        };

        ON_CALL(*mock_buffer_stream, acquire_client_buffer(_))
            .WillByDefault(InvokeArgument<0>(&stub_buffer));
    }

    void TearDown() override
    {
        executor.execute();
    }

    std::shared_ptr<testing::NiceMock<mtd::MockBufferStream>> mock_buffer_stream = std::make_shared<testing::NiceMock<mtd::MockBufferStream>>();
    std::list<ms::StreamInfo> streams{ { mock_buffer_stream, {}, {} } };
    std::function<void()> change_notification;
    int notification_count = 0;
    mtd::StubBuffer stub_buffer;

    std::string surface_name = "test_surfaceA";
    MirPixelFormat pf = mir_pixel_format_abgr_8888;
    geom::Size size = geom::Size{43, 420};
    geom::Stride stride = geom::Stride{4 * size.width.as_uint32_t()};
    geom::Rectangle rect = geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}}, size};
    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    ms::BasicSurface surface;
    std::shared_ptr<mt::doubles::MockEventSink> const mock_event_sink = std::make_shared<mt::doubles::MockEventSink>();
    ms::OutputPropertiesCache cache;
    std::shared_ptr<ms::SurfaceEventSource> observer =
        std::make_shared<ms::SurfaceEventSource>(mf::SurfaceId(), cache, mock_event_sink);
    mtd::ExplicitExecutor executor;
};

}

TEST_F(SurfaceCreation, test_surface_gets_right_name)
{
    EXPECT_EQ(surface_name, surface.name());
}

TEST_F(SurfaceCreation, test_surface_queries_state_for_size)
{
    EXPECT_EQ(size, surface.window_size());
}

TEST_F(SurfaceCreation, test_surface_gets_top_left)
{
    auto ret_top_left = surface.top_left();
    EXPECT_EQ(geom::Point(), ret_top_left);
}

TEST_F(SurfaceCreation, test_surface_move_to)
{
    geom::Point p{55, 66};

    surface.move_to(p);
    EXPECT_EQ(p, surface.top_left());
}

TEST_F(SurfaceCreation, resize_updates_stream_and_state)
{
    using namespace testing;
    geom::Size const new_size{123, 456};

    surface.register_interest(observer, executor);

    ASSERT_THAT(surface.window_size(), Ne(new_size));

    EXPECT_CALL(*mock_event_sink, handle_event(_)).Times(1);
    surface.resize(new_size);
    EXPECT_THAT(surface.window_size(), Eq(new_size));
}

TEST_F(SurfaceCreation, duplicate_resize_ignored)
{
    using namespace testing;
    geom::Size const new_size{123, 456};

    surface.register_interest(observer, executor);

    ASSERT_THAT(surface.window_size(), Ne(new_size));

    EXPECT_CALL(*mock_event_sink, handle_event(_)).Times(1);
    surface.resize(new_size);
    executor.execute();
    EXPECT_THAT(surface.window_size(), Eq(new_size));

    Mock::VerifyAndClearExpectations(mock_buffer_stream.get());
    Mock::VerifyAndClearExpectations(mock_event_sink.get());

    EXPECT_CALL(*mock_event_sink, handle_event(_)).Times(0);
    surface.resize(new_size);
    EXPECT_THAT(surface.window_size(), Eq(new_size));
}

TEST_F(SurfaceCreation, impossible_resize_clamps)
{
    using namespace testing;

    geom::Size const bad_sizes[] =
    {
        {0, 123},
        {456, 0},
        {-1, -1},
        {78, -10},
        {0, 0}
    };

    for (auto &size : bad_sizes)
    {
        geom::Size expect_size = size;
        if (expect_size.width <= geom::Width{0})
            expect_size.width = geom::Width{1};
        if (expect_size.height <= geom::Height{0})
            expect_size.height = geom::Height{1};

        EXPECT_NO_THROW({ surface.resize(size); });
        EXPECT_EQ(expect_size, surface.window_size());
    }
}

TEST_F(SurfaceCreation, consume_calls_send_event)
{
    using namespace testing;

    auto key_event = mev::make_key_event(
        MirInputDeviceId(0), std::chrono::nanoseconds(0),
        mir_keyboard_action_down, 0, 0, mir_input_event_modifier_none);
    auto touch_event = mev::make_touch_event(
        MirInputDeviceId(0), std::chrono::nanoseconds(0), mir_input_event_modifier_none);
    mev::add_touch(*touch_event, 0, mir_touch_action_down, mir_touch_tooltype_finger, 0, 0,
        0, 0, 0, 0);

    surface.register_interest(observer, executor);

    EXPECT_CALL(*mock_event_sink, handle_event(mt::MirKeyboardEventMatches(key_event.get()))).Times(1);
    EXPECT_CALL(*mock_event_sink, handle_event(mt::MirTouchEventMatches(touch_event.get()))).Times(1);

    surface.consume(std::move(key_event));
    surface.consume(std::move(touch_event));
    executor.execute();
}
