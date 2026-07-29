/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
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
#include "mir/test/spin_wait.h"

#include <mir/input/event_builder.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_sink.h>
#include <mir/server.h>
#include <mir/test/signal.h>

#include <miral/append_event_filter.h>
#include <miral/test_server.h>
#include <miral/touch_emulator.h>

#include <mir_toolkit/events/event.h>
#include <mir_toolkit/events/enums.h>
#include <mir_toolkit/events/input/input_event.h>
#include <mir_toolkit/events/input/touch_event.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>
#include <vector>

using namespace testing;
using namespace std::chrono_literals;

namespace mi = mir::input;
namespace geom = mir::geometry;

struct TestTouchEmulator : miral::TestServer
{
    struct CapturedTouch
    {
        auto static to_touch(MirEvent const* event) -> MirTouchEvent const*
        {
            return mir_input_event_get_touch_event(mir_event_get_input_event(event));
        }

        explicit CapturedTouch(MirEvent const* event) :
            action{mir_touch_event_action(to_touch(event), 0)},
            x{mir_touch_event_axis_value(to_touch(event), 0, mir_touch_axis_x)},
            y{mir_touch_event_axis_value(to_touch(event), 0, mir_touch_axis_y)}
        {}

        MirTouchAction const action;
        float const x;
        float const y;
    };

    explicit TestTouchEmulator(
        bool enabled = true,
        std::function<void(mir::Server&)> before_touch_emulator = {}) :
        touch_emulator{enabled ? miral::TouchEmulator::enabled() : miral::TouchEmulator::disabled()}
    {
        add_server_init(
            [this, before_touch_emulator](mir::Server& server)
            {
                server.add_init_callback(
                    [&server, this]
                    {
                        input_device_registry = server.the_input_device_registry();
                        input_device_hub = server.the_input_device_hub();
                    });

                // Stop callbacks run in reverse registration order, so this hook
                // lets lifecycle tests inspect state after the emulator's callback.
                if (before_touch_emulator)
                    before_touch_emulator(server);

                touch_emulator(server);
                observer(server);
            });
    }

    auto add_pointer()
    {
        return miral::test::add_test_device(
            input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::pointer);
    }

    struct PointerEvent
    {
        MirPointerAction action;
        MirPointerButtons buttons;
        float x;
        float y;
    };

    void send_pointer_events(
        std::shared_ptr<mir::input::VirtualInputDevice> const& pointer,
        std::initializer_list<PointerEvent>&& events)
    {
        pointer->if_started_then(
            [&](mi::InputSink* sink, mi::EventBuilder* builder)
            {
                for (auto const& ev : events)
                {
                    sink->handle_input(builder->pointer_event(
                        std::nullopt,
                        ev.action,
                        ev.buttons,
                        std::optional<geom::PointF>{geom::PointF{ev.x, ev.y}},
                        geom::DisplacementF{0.0f, 0.0f},
                        mir_pointer_axis_source_none,
                        {},
                        {}));
                }
            });
    }

    void send_pointer_event(std::shared_ptr<mir::input::VirtualInputDevice> const& pointer, PointerEvent const& event)
    { send_pointer_events(pointer, {event}); }

    auto wait_for_touch_count(size_t expected) -> bool
    {
        return mir::test::spin_wait_for_condition_or_timeout(
            [&] { return touch_events.lock()->size() >= expected; }, 100ms);
    }

    auto captured_touch_events() -> std::vector<CapturedTouch>
    {
        return *touch_events.lock();
    }

    void assert_no_touch()
    {
        EXPECT_THAT(wait_for_touch_count(1), Eq(false));
        EXPECT_THAT(captured_touch_events().size(), Eq(0u));
    }

    std::weak_ptr<mi::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mi::InputDeviceHub> input_device_hub;
    miral::AppendEventFilter observer{
        [this](MirEvent const* event) -> bool
        {
            if (mir_event_get_type(event) != mir_event_type_input)
                return false;
            auto const* input_ev = mir_event_get_input_event(event);
            if (mir_input_event_get_type(input_ev) == mir_input_event_type_touch)
            {
                touch_events.lock()->emplace_back(event);
            }
            return false;
        }};

    miral::TouchEmulator touch_emulator;
    mir::Synchronised<std::vector<CapturedTouch>> touch_events;
};

TEST_F(TestTouchEmulator, generates_touch_events)
{
    auto pointer = add_pointer();

    send_pointer_event(
        pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 100.0f, 200.0f});

    ASSERT_TRUE(wait_for_touch_count(1));
}

struct TestInitiallyDisabledTouchEmulator : TestTouchEmulator
{
    TestInitiallyDisabledTouchEmulator() : TestTouchEmulator{false} {}
};

struct TestInitiallyDisabledButEnabledBeforeServerStartsTouchEmulator : TestTouchEmulator
{
    TestInitiallyDisabledButEnabledBeforeServerStartsTouchEmulator() : TestTouchEmulator{false}
    {
        touch_emulator.enable();
    }
};

TEST_F(TestInitiallyDisabledTouchEmulator, does_not_generate_touch_events_when_disabled)
{
    auto pointer = add_pointer();

    send_pointer_event(
        pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 100.0f, 200.0f});

    assert_no_touch();
}

TEST_F(
    TestInitiallyDisabledButEnabledBeforeServerStartsTouchEmulator,
    generates_touch_events_when_enabled_before_server_starts)
{
    auto pointer = add_pointer();

    send_pointer_event(
        pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 10.0f, 20.0f});

    ASSERT_TRUE(wait_for_touch_count(1));
}

TEST_F(TestInitiallyDisabledTouchEmulator, generates_touch_events_when_enabled_later)
{
    auto pointer = add_pointer();

    send_pointer_event(
        pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 10.0f, 20.0f});
    assert_no_touch();
    send_pointer_event(pointer, {mir_pointer_action_button_up, MirPointerButtons{0}, 10.0f, 20.0f});
    assert_no_touch();

    touch_emulator.enable();

    send_pointer_event(
        pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 30.0f, 40.0f});

    ASSERT_TRUE(wait_for_touch_count(1));
    auto const evts = captured_touch_events();
    ASSERT_THAT(evts.size(), Eq(1u));
    EXPECT_THAT(evts[0].action, Eq(mir_touch_action_down));
    EXPECT_THAT(evts[0].x, FloatEq(30.0f));
    EXPECT_THAT(evts[0].y, FloatEq(40.0f));
}

TEST_F(TestTouchEmulator, keeps_virtual_device_registered_when_disabled)
{
    touch_emulator.disable();

    bool touch_emulator_device_present{false};
    input_device_hub.lock()->for_each_input_device(
        [&touch_emulator_device_present](mi::Device const& device)
        {
            touch_emulator_device_present |= device.name() == "touch-emulator";
        });

    EXPECT_THAT(touch_emulator_device_present, Eq(true));
}

struct TestTouchEmulatorCleanup : TestTouchEmulator
{
    TestTouchEmulatorCleanup() :
        TestTouchEmulator{
            true,
            [this](mir::Server& server)
            {
                server.add_stop_callback(
                    [this]
                    {
                        bool touch_emulator_device_present{false};
                        input_device_hub.lock()->for_each_input_device(
                            [&touch_emulator_device_present](mi::Device const& device)
                            {
                                touch_emulator_device_present |= device.name() == "touch-emulator";
                            });
                        touch_emulator_device_removed = !touch_emulator_device_present;
                    });
            }}
    {
    }

    bool touch_emulator_device_removed{false};
};

TEST_F(TestTouchEmulatorCleanup, removes_device_when_disabled_before_server_stops)
{
    touch_emulator.disable();

    stop_server();

    EXPECT_THAT(touch_emulator_device_removed, Eq(true));
}

TEST_F(TestTouchEmulator, pointer_button_down_generates_touch_down)
{
    auto pointer = add_pointer();

    send_pointer_event(
        pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 100.0f, 200.0f});

    ASSERT_TRUE(wait_for_touch_count(1));
    auto const evts = captured_touch_events();
    ASSERT_THAT(evts.size(), Eq(1u));
    EXPECT_THAT(evts[0].action, Eq(mir_touch_action_down));
    EXPECT_THAT(evts[0].x, FloatEq(100.0f));
    EXPECT_THAT(evts[0].y, FloatEq(200.0f));
}

TEST_F(TestTouchEmulator, drag_generates_touch_change)
{
    auto pointer = add_pointer();

    send_pointer_events(
        pointer,
        {
            {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 100.0f, 200.0f},
            {mir_pointer_action_motion, MirPointerButtons{mir_pointer_button_primary}, 150.0f, 250.0f},
        });

    ASSERT_TRUE(wait_for_touch_count(2));
    auto const evts = captured_touch_events();
    ASSERT_THAT(evts.size(), Ge(2u));
    EXPECT_THAT(evts[1].action, Eq(mir_touch_action_change));
    EXPECT_THAT(evts[1].x, FloatEq(150.0f));
    EXPECT_THAT(evts[1].y, FloatEq(250.0f));
}

TEST_F(TestTouchEmulator, button_up_generates_touch_up)
{
    auto pointer = add_pointer();

    send_pointer_events(
        pointer,
        {
            {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 100.0f, 200.0f},
            {mir_pointer_action_button_up, MirPointerButtons{0}, 100.0f, 200.0f},
        });

    ASSERT_TRUE(wait_for_touch_count(2));
    auto const evts = captured_touch_events();
    ASSERT_THAT(evts.size(), Ge(2u));
    EXPECT_THAT(evts[1].action, Eq(mir_touch_action_up));
    EXPECT_THAT(evts[1].x, FloatEq(100.0f));
    EXPECT_THAT(evts[1].y, FloatEq(200.0f));
}

TEST_F(TestTouchEmulator, full_gesture_produces_down_change_up)
{
    auto pointer = add_pointer();

    send_pointer_events(
        pointer,
        {
            {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 10.0f, 20.0f},
            {mir_pointer_action_motion, MirPointerButtons{mir_pointer_button_primary}, 30.0f, 40.0f},
            {mir_pointer_action_button_up, MirPointerButtons{0}, 30.0f, 40.0f},
        });

    ASSERT_TRUE(wait_for_touch_count(3));
    auto const evts = captured_touch_events();
    ASSERT_THAT(evts.size(), Eq(3u));
    EXPECT_THAT(evts[0].action, Eq(mir_touch_action_down));
    EXPECT_THAT(evts[0].x, FloatEq(10.0f));
    EXPECT_THAT(evts[0].y, FloatEq(20.0f));
    EXPECT_THAT(evts[1].action, Eq(mir_touch_action_change));
    EXPECT_THAT(evts[1].x, FloatEq(30.0f));
    EXPECT_THAT(evts[1].y, FloatEq(40.0f));
    EXPECT_THAT(evts[2].action, Eq(mir_touch_action_up));
    EXPECT_THAT(evts[2].x, FloatEq(30.0f));
    EXPECT_THAT(evts[2].y, FloatEq(40.0f));
}

TEST_F(TestTouchEmulator, non_primary_button_does_not_generate_touch)
{
    auto pointer = add_pointer();

    send_pointer_events(
        pointer,
        {
            {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_secondary}, 100.0f, 200.0f},
            {mir_pointer_action_button_up, MirPointerButtons{0}, 100.0f, 200.0f},
        });

    assert_no_touch();
}

TEST_F(TestTouchEmulator, no_touch_events_when_disabled)
{
    touch_emulator.disable();

    auto pointer = add_pointer();

    send_pointer_event(
        pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, 100.0f, 200.0f});

    assert_no_touch();
}

TEST_F(TestTouchEmulator, disable_and_re_enable_toggles_behavior)
{
    auto pointer = add_pointer();

    auto const click = [&](auto x, auto y, auto const& check_down, auto const& check_up)
    {
        send_pointer_event(
            pointer, {mir_pointer_action_button_down, MirPointerButtons{mir_pointer_button_primary}, x, y});

        check_down();

        send_pointer_event(pointer, {mir_pointer_action_button_up, MirPointerButtons{0}, x, y});

        check_up();
    };

    click(
        10.0f,
        20.0f,
        [this]
        {
            ASSERT_TRUE(wait_for_touch_count(1));
        },
        [this]
        {
            ASSERT_TRUE(wait_for_touch_count(2));
        });

    touch_emulator.disable();
    touch_events.lock_mut()->clear();
    ASSERT_THAT(captured_touch_events().size(), Eq(0u));

    click(30.0f, 40.0f, [this] { assert_no_touch(); }, [this] { assert_no_touch(); });

    touch_emulator.enable();
    touch_events.lock()->clear();

    click(
        50.0f,
        60.0f,
        [this] { ASSERT_TRUE(wait_for_touch_count(1)); },
        [this] { ASSERT_TRUE(wait_for_touch_count(2)); });

    auto const evts = captured_touch_events();
    ASSERT_THAT(evts.size(), Eq(2u));
    EXPECT_THAT(evts[0].action, Eq(mir_touch_action_down));
    EXPECT_THAT(evts[0].x, FloatEq(50.0f));
    EXPECT_THAT(evts[0].y, FloatEq(60.0f));
    EXPECT_THAT(evts[1].action, Eq(mir_touch_action_up));
    EXPECT_THAT(evts[1].x, FloatEq(50.0f));
    EXPECT_THAT(evts[1].y, FloatEq(60.0f));
}
