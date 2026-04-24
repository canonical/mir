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

#include "add_virtual_device.h"

#include <mir/input/composite_event_filter.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_sink.h>
#include <mir/input/virtual_input_device.h>
#include <mir/server.h>
#include <mir/test/signal.h>

#include <miral/append_event_filter.h>
#include <miral/locate_pointer.h>
#include <miral/test_server.h>

#include <gmock/gmock.h>

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
static constexpr auto locate_pointer_delay = 100ms;
// Accounts for CI slowness - provides ample time for the alarm to fire after scheduling
static constexpr auto locate_pointer_callback_timeout = std::chrono::seconds{5};
}

struct TestLocatePointer : miral::TestServer
{
    TestLocatePointer()
    {
        add_server_init(
            [this](mir::Server& server)
            {
                server.add_init_callback(
                    [&server, this]
                    {
                        composite_event_filter = server.the_composite_event_filter();
                        input_device_registry = server.the_input_device_registry();
                        input_device_hub = server.the_input_device_hub();
                    });
            });

        add_server_init(locate_pointer);

        locate_pointer
            .delay(locate_pointer_delay)
            .on_locate_pointer([this](auto...) { locate_pointer_invoked.raise(); });
    }

    void SetUp() override
    {
        miral::TestServer::SetUp();
    }

    miral::LocatePointer locate_pointer{miral::LocatePointer::enabled()};
    std::shared_ptr<mir::input::VirtualInputDevice> test_device = std::make_shared<mir::input::VirtualInputDevice>(
        "test-device", mir::input::DeviceCapability::keyboard | mir::input::DeviceCapability::pointer);
    mir::test::Signal locate_pointer_invoked;

    std::weak_ptr<mir::input::CompositeEventFilter> composite_event_filter;
    std::weak_ptr<mir::input::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mir::input::InputDeviceHub> input_device_hub;
};

struct TestDifferentLocatePointerDelays : public TestLocatePointer, public WithParamInterface<std::pair<std::chrono::milliseconds, bool>>{};

TEST_P(TestDifferentLocatePointerDelays, responding_to_schedule_and_request_with_different_delays)
{
    auto [delay, on_pointer_locate_called] = GetParam();

    locate_pointer.schedule_request();
    locate_pointer_invoked.wait_for(delay);
    locate_pointer.cancel_request();

    EXPECT_THAT(locate_pointer_invoked.raised(), Eq(on_pointer_locate_called));
}

INSTANTIATE_TEST_SUITE_P(
    TestLocatePointer,
    TestDifferentLocatePointerDelays,
    ::Values(std::pair{locate_pointer_delay + 10ms, true}, std::pair{locate_pointer_delay - 10ms, false}));

struct TestPointerMovementTracking :
    public TestLocatePointer,
    public WithParamInterface<std::optional<mir::geometry::PointF>>
{
};

TEST_P(TestPointerMovementTracking, cursor_location_is_tracked_on_movement)
{
    auto const final_pointer_position = GetParam();

    std::optional<mir::geometry::PointF> locate_pointer_position;
    locate_pointer.on_locate_pointer(
        [this, &locate_pointer_position](auto pointer_position)
        {
            locate_pointer_position = pointer_position;
            locate_pointer_invoked.raise();
        });

    auto test_device = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mir::input::DeviceCapability::pointer);
    test_device->if_started_then(
        [this, final_pointer_position](auto* sink, auto* builder)
        {
            sink->handle_input(builder->pointer_event(
                std::nullopt,
                mir_pointer_action_motion,
                MirPointerButtons{0},
                final_pointer_position,
                mir::geometry::DisplacementF{0.0f, 0.0f},
                mir_pointer_axis_source_none,
                {},
                {}));
            locate_pointer.schedule_request();
        });

    EXPECT_TRUE(locate_pointer_invoked.wait_for(locate_pointer_callback_timeout));

    auto const expected_position = final_pointer_position.value_or(mir::geometry::PointF{0, 0});
    EXPECT_THAT(locate_pointer_position, Optional(expected_position));
}

INSTANTIATE_TEST_SUITE_P(
    TestLocatePointer, TestPointerMovementTracking, ::Values(std::nullopt, mir::geometry::PointF{100.0f, 100.0f}));
