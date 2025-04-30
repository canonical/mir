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

#include "mir/input/event_builder.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/virtual_input_device.h"
#include "mir/server.h"
#include "mir/test/signal.h"

#include "miral/locate_pointer.h"
#include "miral/test_server.h"
#include "miral/toolkit_event.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
static constexpr auto locate_pointer_delay = 100ms;
static constexpr auto enable_or_disable_delay = 50ms; // Should be plenty for the main loop to execute even on CI

mir::EventUPtr left_ctrl_down_event(mir::input::EventBuilder* builder)
{
    return builder->key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_Control_L, 29);
}
}

struct TestLocatePointer : miral::TestServer
{
    TestLocatePointer()
    {
        add_server_init(
            [this](mir::Server& server)
            {
                server.add_init_callback([this, &server]
                                         { server.the_input_device_registry()->add_device(test_device); });
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
    std::shared_ptr<mir::input::VirtualInputDevice> test_device = std::make_shared<mir::input::VirtualInputDevice>(
        "test-device", mir::input::DeviceCapability::keyboard | mir::input::DeviceCapability::pointer);
    mir::test::Signal locate_pointer_invoked, locate_pointer_enabled;
};

struct TestDifferentDelays : public TestLocatePointer, public WithParamInterface<std::pair<std::chrono::milliseconds, bool>>{};

TEST_P(TestDifferentDelays, responding_to_ctrl_down_with_different_delays)
{
    auto [delay, on_pointer_locate_called] = GetParam();

    std::chrono::nanoseconds up_event_time;
    test_device->if_started_then(
        [&up_event_time, delay](auto* sink, auto* builder)
        {
            auto down_event = left_ctrl_down_event(builder);
            up_event_time = std::chrono::nanoseconds{mir_input_event_get_event_time(
                                           reinterpret_cast<MirInputEvent*>(down_event.get()))} +
                                       delay;

            sink->handle_input(std::move(down_event));
        });
    locate_pointer_invoked.wait_for(delay);
    test_device->if_started_then(
        [up_event_time](auto* sink, auto* builder)
        {
            auto up_event = builder->key_event(up_event_time, mir_keyboard_action_up, XKB_KEY_Control_L, 29);
            sink->handle_input(std::move(up_event));
        });

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

    test_device->if_started_then(
        [final_pointer_position](auto* sink, auto* builder)
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
            sink->handle_input(left_ctrl_down_event(builder));
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
    mir::test::Signal enable_called;
    locate_pointer.on_enabled([&enable_called] { enable_called.raise(); });
    locate_pointer.enable();

    enable_called.wait_for(enable_or_disable_delay);
    EXPECT_FALSE(enable_called.raised());
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

TEST_F(TestLocatePointer, key_combination_nothing_passes)
{
    locate_pointer.key_combination([](auto){ return false; }); // Nothing will trigger locate pointer
    test_device->if_started_then([](auto* sink, auto* builder){ sink->handle_input(left_ctrl_down_event(builder)); });

    locate_pointer_invoked.wait_for(locate_pointer_delay);
    EXPECT_FALSE(locate_pointer_invoked.raised());
}

TEST_F(TestLocatePointer, key_combination_anything_passes)
{
    locate_pointer.key_combination([](auto){ return true; }); // Anything will trigger locate pointer
    test_device->if_started_then([](auto* sink, auto* builder) { sink->handle_input(left_ctrl_down_event(builder)); });

    locate_pointer_invoked.wait();
    EXPECT_TRUE(locate_pointer_invoked.raised());
}

// Default key combination filter is tested above
TEST_F(TestLocatePointer, non_default_key_combination_filters_correctly)
{
    namespace mtk = miral::toolkit;
    // ctrl + alt + shift + p
    locate_pointer.key_combination(
        [](auto input_event)
        {
            auto const keyboard_event = mtk::mir_input_event_get_keyboard_event(input_event);
            if (!keyboard_event)
                return false;

            auto const keysym = mtk::mir_keyboard_event_keysym(keyboard_event);
            // Uppcase P because having shift down transforms lowercase p to uppercase
            if (keysym != XKB_KEY_P)
                return false;

            auto const modifiers = mtk::mir_keyboard_event_modifiers(keyboard_event);
            if (!(modifiers & mir_input_event_modifier_ctrl && modifiers & mir_input_event_modifier_alt &&
                  modifiers & mir_input_event_modifier_shift))
                return false;

            return true;
        });

    test_device->if_started_then(
        [](auto* sink, auto* builder)
        {
            sink->handle_input(left_ctrl_down_event(builder));
            sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_Alt_L, 56));
            sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_Shift_L, 42));
            sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_p, 25));
        });

    locate_pointer_invoked.wait();
    EXPECT_TRUE(locate_pointer_invoked.raised());
}
