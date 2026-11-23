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

#include "src/server/input/cursor_hiding_dispatcher.h"

#include <mir/events/keyboard_event.h>
#include <mir/events/pointer_event.h>
#include <mir/events/touch_event.h>
#include <mir/input/device.h>
#include <mir/input/device_capability.h>
#include <mir/input/mir_keyboard_config.h>
#include <mir/input/mir_pointer_config.h>
#include <mir/input/mir_touchpad_config.h>
#include <mir/input/mir_touchscreen_config.h>

#include <mir/test/fake_shared.h>
#include <mir/test/doubles/advanceable_clock.h>
#include <mir/test/doubles/fake_alarm_factory.h>
#include <mir/test/doubles/mock_input_dispatcher.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
struct StubDevice : mi::Device
{
    StubDevice(MirInputDeviceId id, std::string name, mi::DeviceCapabilities caps)
        : device_id{id}, device_name{std::move(name)}, caps{caps}
    {
    }

    MirInputDeviceId id() const override { return device_id; }
    mi::DeviceCapabilities capabilities() const override { return caps; }
    std::string name() const override { return device_name; }
    std::string unique_id() const override { return {}; }
    std::optional<MirPointerConfig> pointer_configuration() const override { return {}; }
    void apply_pointer_configuration(MirPointerConfig const&) override {}
    std::optional<MirTouchpadConfig> touchpad_configuration() const override { return {}; }
    void apply_touchpad_configuration(MirTouchpadConfig const&) override {}
    std::optional<MirKeyboardConfig> keyboard_configuration() const override { return {}; }
    void apply_keyboard_configuration(MirKeyboardConfig const&) override {}
    std::optional<MirTouchscreenConfig> touchscreen_configuration() const override { return {}; }
    void apply_touchscreen_configuration(MirTouchscreenConfig const&) override {}

    MirInputDeviceId const device_id;
    std::string const device_name;
    mi::DeviceCapabilities const caps;
};

auto touch_down(MirInputDeviceId device_id = 1) -> std::shared_ptr<MirEvent>
{
    auto const ev = std::make_shared<MirTouchEvent>();
    ev->set_device_id(device_id);
    ev->set_pointer_count(1);
    ev->set_action(0, mir_touch_action_down);
    ev->set_position(0, {});
    return ev;
}

auto key_down() -> std::shared_ptr<MirEvent>
{
    auto const ev = std::make_shared<MirKeyboardEvent>();
    ev->set_action(mir_keyboard_action_down);
    return ev;
}

auto pointer(
    geom::PointF position,
    MirPointerAction action = mir_pointer_action_motion,
    MirPointerButtons buttons = 0) -> std::shared_ptr<MirEvent>
{
    auto const ev = std::make_shared<MirPointerEvent>();
    ev->set_action(action);
    ev->set_buttons(buttons);
    ev->set_position(position);
    return ev;
}

struct CursorHidingDispatcher : Test
{
    void advance_by(std::chrono::milliseconds step)
    {
        clock.advance_by(step);
        alarm_factory.advance_by(step);
    }

    void dispatch(std::shared_ptr<MirEvent> const& event)
    {
        dispatcher.dispatch(event);
    }

    NiceMock<mtd::MockInputDispatcher> next_dispatcher;
    NiceMock<MockFunction<void(bool)>> set_cursor_hidden;
    bool pointer_constrained{false};
    mtd::AdvanceableClock clock;
    mtd::FakeAlarmFactory alarm_factory;
    mi::CursorHidingDispatcher dispatcher{
        mt::fake_shared(next_dispatcher),
        set_cursor_hidden.AsStdFunction(),
        [this] { return pointer_constrained; },
        mt::fake_shared(clock),
        alarm_factory};
};
}

TEST_F(CursorHidingDispatcher, touch_hides_cursor_only_when_enabled)
{
    EXPECT_CALL(set_cursor_hidden, Call(_)).Times(0);
    dispatch(touch_down());
    Mock::VerifyAndClearExpectations(&set_cursor_hidden);

    dispatcher.hide_on_touch(true);
    EXPECT_CALL(set_cursor_hidden, Call(true));
    dispatch(touch_down());
}

TEST_F(CursorHidingDispatcher, touch_from_touch_emulator_hides_cursor)
{
    MirInputDeviceId const emulator_id{7};
    dispatcher.hide_on_touch(true);
    dispatcher.device_added(std::make_shared<StubDevice>(
        emulator_id, "touch-emulator", mi::DeviceCapability::touchscreen));

    EXPECT_CALL(set_cursor_hidden, Call(true));
    dispatch(touch_down(emulator_id));
}

TEST_F(CursorHidingDispatcher, key_press_hides_cursor_when_enabled)
{
    dispatcher.hide_on_keyboard(true);

    EXPECT_CALL(set_cursor_hidden, Call(true));
    dispatch(key_down());
}

TEST_F(CursorHidingDispatcher, inactivity_hides_cursor_after_timeout)
{
    dispatcher.hide_after_inactivity(5s);

    EXPECT_CALL(set_cursor_hidden, Call(_)).Times(0);
    advance_by(3s);
    dispatch(pointer({0, 0}));
    advance_by(3s);
    Mock::VerifyAndClearExpectations(&set_cursor_hidden);

    EXPECT_CALL(set_cursor_hidden, Call(true));
    advance_by(2s);
}

TEST_F(CursorHidingDispatcher, held_button_or_pointer_constraint_prevents_hiding)
{
    dispatcher.hide_on_touch(true);
    EXPECT_CALL(set_cursor_hidden, Call(_)).Times(0);

    dispatch(pointer({0, 0}, mir_pointer_action_button_down, mir_pointer_button_primary));
    dispatch(touch_down());

    dispatch(pointer({0, 0}, mir_pointer_action_button_up, 0));
    pointer_constrained = true;
    dispatch(touch_down());
}

TEST_F(CursorHidingDispatcher, motion_beyond_threshold_reveals_cursor)
{
    dispatcher.hide_on_touch(true);
    dispatch(pointer({0, 0}));
    dispatch(touch_down());

    EXPECT_CALL(set_cursor_hidden, Call(false)).Times(0);
    dispatch(pointer({3, 4}));
    Mock::VerifyAndClearExpectations(&set_cursor_hidden);

    EXPECT_CALL(set_cursor_hidden, Call(false));
    dispatch(pointer({6, 8}));
}

TEST_F(CursorHidingDispatcher, button_press_reveals_cursor)
{
    dispatcher.hide_on_touch(true);
    dispatch(touch_down());

    EXPECT_CALL(set_cursor_hidden, Call(false));
    dispatch(pointer({0, 0}, mir_pointer_action_button_down, mir_pointer_button_primary));
}
