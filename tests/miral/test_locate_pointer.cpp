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

#include "mir/input/composite_event_filter.h"
#include "mir/input/device.h"
#include "mir/input/event_builder.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/virtual_input_device.h"
#include "mir/server.h"
#include "mir/test/signal.h"

#include "miral/append_event_filter.h"
#include "miral/locate_pointer.h"
#include "miral/test_server.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
static constexpr auto locate_pointer_delay = 100ms;
static constexpr auto enable_or_disable_delay = 50ms; // Should be plenty for the main loop to execute even on CI

// Mostly to account for CI slowness.
// Adds a test pointer and waits till it's added before proceeding
std::shared_ptr<mir::input::VirtualInputDevice> add_test_pointer(
    std::weak_ptr<mir::input::InputDeviceRegistry> const& input_device_registry,
    std::weak_ptr<mir::input::InputDeviceHub> const& input_device_hub)
{
    std::shared_ptr<mir::input::VirtualInputDevice> test_device =
        std::make_shared<mir::input::VirtualInputDevice>("test-device", mir::input::DeviceCapability::pointer);

    struct SignalingObserver : public mir::input::InputDeviceObserver
    {
        using Device = mir::input::Device;
        std::shared_ptr<Device> const device_to_wait_for;
        std::weak_ptr<mir::input::InputDeviceHub> const device_hub;
        mir::test::Signal device_added_;

        SignalingObserver(std::shared_ptr<Device> const& to_wait_for) :
            device_to_wait_for{to_wait_for}
        {
        }

        virtual void device_added(std::shared_ptr<Device> const& device) override
        {
            if (device->unique_id() == device_to_wait_for->unique_id())
                device_added_.raise();
        }

        virtual void device_changed(std::shared_ptr<Device> const&)
        {
        }
        virtual void device_removed(std::shared_ptr<Device> const&)
        {
        }
        virtual void changes_complete()
        {
        }

        void wait()
        {
            device_added_.wait();
        }
    };

    auto input_device_observer =
        std::make_shared<SignalingObserver>(input_device_registry.lock()->add_device(test_device).lock());
    input_device_hub.lock()->add_observer(input_device_observer);
    input_device_observer->wait();
    input_device_observer.reset();

    return test_device;
}
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
            .on_locate_pointer([this](auto...) { locate_pointer_invoked.raise(); })
            .on_enabled([this](auto...){ locate_pointer_enabled.raise(); });
    }

    void SetUp() override
    {
        miral::TestServer::SetUp();
        locate_pointer_enabled.wait();
    }

    miral::LocatePointer locate_pointer{true};
    mir::test::Signal locate_pointer_invoked, locate_pointer_enabled;

    std::weak_ptr<mir::input::CompositeEventFilter> composite_event_filter;
    std::weak_ptr<mir::input::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mir::input::InputDeviceHub> input_device_hub;
};

struct TestDifferentDelays : public TestLocatePointer, public WithParamInterface<std::pair<std::chrono::milliseconds, bool>>{};

TEST_P(TestDifferentDelays, responding_to_ctrl_down_with_different_delays)
{
    auto [delay, on_pointer_locate_called] = GetParam();

    locate_pointer.schedule_request();
    locate_pointer_invoked.wait_for(delay);
    locate_pointer.cancel_request();

    EXPECT_THAT(locate_pointer_invoked.raised(), Eq(on_pointer_locate_called));
}

INSTANTIATE_TEST_SUITE_P(
    TestLocatePointer,
    TestDifferentDelays,
    ::Values(std::pair{locate_pointer_delay + 10ms, true}, std::pair{locate_pointer_delay - 10ms, false}));

struct TestPointerMovementTracking :
    public TestLocatePointer,
    public WithParamInterface<std::optional<mir::geometry::PointF>>
{
};

TEST_P(TestPointerMovementTracking, cursor_location_is_tracked_on_movement)
{
    auto const final_pointer_position = GetParam();

    std::optional<float> locate_pointer_x = std::nullopt, locate_pointer_y = std::nullopt;
    locate_pointer.on_locate_pointer(
        [this, &locate_pointer_x, &locate_pointer_y](auto x, auto y)
        {
            locate_pointer_x = x;
            locate_pointer_y = y;
            locate_pointer_invoked.raise();
        });

    auto test_device = add_test_pointer(input_device_registry, input_device_hub);
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

    locate_pointer_invoked.wait_for(locate_pointer_delay + 10ms);

    auto const expected_position = final_pointer_position.value_or(mir::geometry::PointF{0, 0});
    EXPECT_THAT(locate_pointer_x, Optional(expected_position.x.as_value()));
    EXPECT_THAT(locate_pointer_y, Optional(expected_position.y.as_value()));
}

INSTANTIATE_TEST_SUITE_P(
    TestLocatePointer, TestPointerMovementTracking, ::Values(std::nullopt, mir::geometry::PointF{100.0f, 100.0f}));

TEST_F(TestLocatePointer, enable_not_called_when_already_enabled)
{
    locate_pointer_enabled.reset();
    locate_pointer.enable();

    locate_pointer_enabled.wait_for(enable_or_disable_delay);
    EXPECT_FALSE(locate_pointer_enabled.raised());
}

TEST_F(TestLocatePointer, disable_called_if_enabled)
{
    mir::test::Signal disable_called;
    locate_pointer.on_disabled([&disable_called] { disable_called.raise(); });
    locate_pointer.disable();

    disable_called.wait();
    EXPECT_TRUE(disable_called.raised());
}

TEST_F(TestLocatePointer, disable_not_called_if_already_disabled)
{
    locate_pointer.disable();

    mir::test::Signal disable_called;
    locate_pointer.on_disabled([&disable_called] { disable_called.raise(); });
    locate_pointer.disable();

    disable_called.wait_for(enable_or_disable_delay);
    EXPECT_FALSE(disable_called.raised());
}

TEST_F(TestLocatePointer, enable_called_if_disabled)
{
    locate_pointer.disable();

    mir::test::Signal enable_called;
    locate_pointer.on_enabled([&enable_called] { enable_called.raise(); });
    locate_pointer.enable();

    enable_called.wait();
    EXPECT_TRUE(enable_called.raised());
}

