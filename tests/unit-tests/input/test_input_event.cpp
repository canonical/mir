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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/events/event_private.h"
#include "mir_toolkit/events/input/input_event.h"

using namespace testing;

TEST(InputEvent, old_style_key_and_motion_events_are_input_events)
{
    MirKeyboardEvent key_ev;

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(&key_ev));
    EXPECT_EQ(mir_input_event_type_key, mir_input_event_get_type(mir_event_get_input_event(&key_ev)));

    MirTouchEvent motion_ev;

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(&motion_ev));
    EXPECT_NE(nullptr, mir_event_get_input_event(&motion_ev));
}

// MirInputEvent properties common to all events.

TEST(CommonInputEventProperties, device_id_taken_from_old_style_event)
{
    int64_t dev_id_1 = 17, dev_id_2 = 23;
    MirTouchEvent old_ev;

    old_ev.set_device_id(dev_id_1);
    EXPECT_EQ(dev_id_1, mir_input_event_get_device_id(
        mir_event_get_input_event(&old_ev)));

    MirKeyboardEvent kev;
    kev.set_device_id(dev_id_2);
    EXPECT_EQ(dev_id_2, mir_input_event_get_device_id(
        mir_event_get_input_event(&kev)));
}

TEST(CommonInputEventProperties, event_time_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    MirTouchEvent old_ev;

    old_ev.set_event_time(event_time_1);
    EXPECT_EQ(event_time_1.count(), mir_input_event_get_event_time(
        mir_event_get_input_event(&old_ev)));

    MirKeyboardEvent kev;
    kev.set_event_time(event_time_2);
    EXPECT_EQ(event_time_2.count(), mir_input_event_get_event_time(
        mir_event_get_input_event(&kev)));
}

TEST(KeyInputEventProperties, timestamp_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    MirKeyboardEvent old_ev;
    auto const keyboard_event = mir_input_event_get_keyboard_event(mir_event_get_input_event(&old_ev));

    for (auto expected : {event_time_1, event_time_2})
    {
        old_ev.set_event_time(expected);

        auto const input_event = mir_keyboard_event_input_event(keyboard_event);

        EXPECT_THAT(mir_input_event_get_event_time(input_event), Eq(expected.count()));
    }
}


TEST(KeyInputEventProperties, up_and_down_actions_copied_from_old_style_event)
{
    MirKeyboardEvent old_ev;

    old_ev.set_action(mir_keyboard_action_down);

    auto new_kev = mir_input_event_get_keyboard_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_keyboard_action_down, mir_keyboard_event_action(new_kev));

    old_ev.set_action(mir_keyboard_action_up);
    EXPECT_EQ(mir_keyboard_action_up, mir_keyboard_event_action(new_kev));
}

TEST(KeyInputEventProperties, keysym_scancode_and_modifiers_taken_from_old_style_event)
{
    xkb_keysym_t keysym = 171;
    int scan_code = 31;
    MirInputEventModifiers modifiers = mir_input_event_modifier_shift;

    MirKeyboardEvent old_ev;
    old_ev.set_keysym(keysym);
    old_ev.set_scan_code(scan_code);
    old_ev.set_modifiers(modifiers);

    auto new_kev = mir_input_event_get_keyboard_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(keysym, mir_keyboard_event_keysym(new_kev));
    EXPECT_EQ(scan_code, mir_keyboard_event_scan_code(new_kev));
    EXPECT_EQ(modifiers, mir_keyboard_event_modifiers(new_kev));
}

TEST(TouchEventProperties, timestamp_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    MirTouchEvent old_ev;
    auto const touch_event = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));

    for (auto expected : {event_time_1, event_time_2})
    {
        old_ev.set_event_time(expected);

        auto const input_event = mir_touch_event_input_event(touch_event);

        EXPECT_THAT(mir_input_event_get_event_time(input_event), Eq(expected.count()));
    }
}

TEST(TouchEventProperties, touch_count_taken_from_pointer_count)
{
    unsigned const pointer_count = 3;
    MirTouchEvent old_ev;

    old_ev.set_pointer_count(pointer_count);

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(pointer_count, mir_touch_event_point_count(tev));
}

TEST(TouchEventProperties, touch_id_comes_from_pointer_coordinates)
{
    int const touch_id = 31;
    MirTouchEvent old_ev;

    old_ev.set_pointer_count(1);
    old_ev.set_id(0, touch_id);

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(touch_id, mir_touch_event_id(tev, 0));
}

// mir_motion_action_up/down represent the start of a gesture. pointers only go up/down one at a time
TEST(TouchEventProperties, down_and_up_actions_are_taken_from_old_event)
{
    MirTouchEvent old_ev;
    old_ev.set_pointer_count(1);
    old_ev.set_action(0, mir_touch_action_change);

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_touch_action_change, mir_touch_event_action(tev, 0));
}

TEST(TouchEventProperties, tool_type_copied_from_old_pc)
{
    MirTouchEvent old_ev;

    old_ev.set_pointer_count(3);
    old_ev.set_tool_type(0, mir_touch_tooltype_unknown);
    old_ev.set_tool_type(1, mir_touch_tooltype_finger);
    old_ev.set_tool_type(2, mir_touch_tooltype_stylus);

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_touch_tooltype_unknown, mir_touch_event_tooltype(tev, 0));
    EXPECT_EQ(mir_touch_tooltype_finger, mir_touch_event_tooltype(tev, 1));
    EXPECT_EQ(mir_touch_tooltype_stylus, mir_touch_event_tooltype(tev, 2));
}

TEST(TouchEventProperties, axis_values_used_by_qtmir_copied)
{
    float x_value = 19, y_value = 23, touch_major = .3, touch_minor = .2, pressure = .9;
    MirTouchEvent old_ev;
    old_ev.set_pointer_count(1);
    old_ev.set_position(0, {x_value, y_value});
    old_ev.set_touch_major(0, touch_major);
    old_ev.set_touch_minor(0, touch_minor);
    old_ev.set_pressure(0, pressure);

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(x_value, mir_touch_event_axis_value(tev, 0, mir_touch_axis_x));
    EXPECT_EQ(y_value, mir_touch_event_axis_value(tev, 0, mir_touch_axis_y));
    EXPECT_EQ(touch_major, mir_touch_event_axis_value(tev, 0, mir_touch_axis_touch_major));
    EXPECT_EQ(touch_minor, mir_touch_event_axis_value(tev, 0, mir_touch_axis_touch_minor));
    EXPECT_EQ(pressure, mir_touch_event_axis_value(tev, 0, mir_touch_axis_pressure));
}

TEST(PointerInputEventProperties, timestamp_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    MirPointerEvent old_ev;
    auto const pointer_event = mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));

    for (auto expected : {event_time_1, event_time_2})
    {
        old_ev.set_event_time(expected);

        auto const input_event = mir_pointer_event_input_event(pointer_event);

        EXPECT_THAT(mir_input_event_get_event_time(input_event), Eq(expected.count()));
    }
}

TEST(PointerInputEventProperties, modifiers_taken_from_old_style_ev)
{
    MirInputEventModifiers modifiers = mir_input_event_modifier_shift;
    MirPointerEvent old_ev;
    old_ev.set_modifiers(modifiers);

    auto pointer_event =
        mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(modifiers, mir_pointer_event_modifiers(pointer_event));
}

TEST(PointerInputEventProperties, button_state_translated)
{
    MirPointerEvent old_ev;

    old_ev.set_buttons(mir_pointer_button_primary);
    auto pev = mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));

    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_primary));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_secondary));

    old_ev.set_buttons(old_ev.buttons() | mir_pointer_button_secondary);

    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_primary));
    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_secondary));

    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_tertiary));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_back));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_forward));
}

TEST(PointerInputEventProperties, axis_values_copied)
{
    float x = 7, y = 9.3, hscroll = 13, vscroll = 17;
    MirPointerEvent old_ev;
    old_ev.set_position({{x, y}});
    old_ev.set_h_scroll({mir::geometry::DeltaXF{hscroll}, {}, false});
    old_ev.set_v_scroll({mir::geometry::DeltaYF{vscroll}, {}, false});

    auto pev = mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(x, mir_pointer_event_axis_value(pev, mir_pointer_axis_x));
    EXPECT_EQ(y, mir_pointer_event_axis_value(pev, mir_pointer_axis_y));
    EXPECT_EQ(vscroll, mir_pointer_event_axis_value(pev, mir_pointer_axis_vscroll));
    EXPECT_EQ(hscroll, mir_pointer_event_axis_value(pev, mir_pointer_axis_hscroll));
}
