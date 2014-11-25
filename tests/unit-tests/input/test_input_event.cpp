/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include <gtest/gtest.h>

#include "mir_toolkit/event.h"
#include "mir_toolkit/input/input_event.h"

// See: https://bugs.launchpad.net/mir/+bug/1311699
#define MIR_EVENT_ACTION_POINTER_INDEX_MASK 0xff00
#define MIR_EVENT_ACTION_POINTER_INDEX_SHIFT 8;

TEST(InputEvent, old_style_key_and_motion_events_are_input_events)
{
    MirEvent key_ev;
    key_ev.type = mir_event_type_key;
    
    EXPECT_EQ(mir_event_type_input, mir_event_get_type(&key_ev));
    EXPECT_EQ(mir_input_event_type_key, mir_input_event_get_type(mir_event_get_input_event(&key_ev)));

    MirEvent motion_ev;
    motion_ev.type = mir_event_type_motion;
    
    EXPECT_EQ(mir_event_type_input, mir_event_get_type(&motion_ev));
    EXPECT_NE(nullptr, mir_event_get_input_event(&motion_ev));
}

// MirInputEvent properties common to all events.

TEST(CommonInputEventProperties, device_id_taken_from_old_style_event)
{
    int64_t dev_id_1 = 17, dev_id_2 = 23;
    MirEvent old_ev;

    old_ev.type = mir_event_type_motion;
    old_ev.motion.device_id = dev_id_1;
    EXPECT_EQ(dev_id_1, mir_input_event_get_device_id(
        mir_event_get_input_event(&old_ev)));

    old_ev.type = mir_event_type_key;
    old_ev.motion.device_id = dev_id_2;
    EXPECT_EQ(dev_id_2, mir_input_event_get_device_id(
        mir_event_get_input_event(&old_ev)));
}

TEST(CommonInputEventProperties, event_time_taken_from_old_style_event)
{
    int64_t event_time_1 = 79, event_time_2 = 83;
    MirEvent old_ev;
    
    old_ev.type = mir_event_type_motion;
    old_ev.motion.event_time = event_time_1;
    EXPECT_EQ(event_time_1, mir_input_event_get_event_time(
        mir_event_get_input_event(&old_ev)));

    old_ev.type = mir_event_type_key;
    old_ev.key.event_time = event_time_2;
    EXPECT_EQ(event_time_2, mir_input_event_get_event_time(
        mir_event_get_input_event(&old_ev)));
}

TEST(KeyInputEventProperties, up_and_down_actions_copied_from_old_style_event)
{
    MirEvent old_ev;
    old_ev.type = mir_event_type_key;

    old_ev.key.action = mir_key_action_down;
    old_ev.key.repeat_count = 0;
    
    auto new_kev = mir_input_event_get_key_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_key_input_event_action_down, mir_key_input_event_get_action(new_kev));

    old_ev.key.action = mir_key_action_up;
    EXPECT_EQ(mir_key_input_event_action_up, mir_key_input_event_get_action(new_kev));
}

TEST(KeyInputEventProperties, repeat_action_produced_from_non_zero_repeat_count_in_old_style_event)
{
    MirEvent old_ev;
    old_ev.type = mir_event_type_key;
    old_ev.key.action = mir_key_action_down;
    old_ev.key.repeat_count = 1;

    auto new_kev = mir_input_event_get_key_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_key_input_event_action_repeat, mir_key_input_event_get_action(new_kev));
}

TEST(KeyInputEventProperties, keycode_scancode_and_modifiers_taken_from_old_style_event)
{
    xkb_keysym_t key_code = 171;
    int scan_code = 31;
    MirKeyModifier old_modifiers = mir_key_modifier_shift;

    MirEvent old_ev;
    old_ev.type = mir_event_type_key;
    old_ev.key.key_code = key_code;
    old_ev.key.scan_code = scan_code;
    old_ev.key.modifiers = old_modifiers;

    auto new_kev = mir_input_event_get_key_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(key_code, mir_key_input_event_get_key_code(new_kev));
    EXPECT_EQ(scan_code, mir_key_input_event_get_scan_code(new_kev));
    EXPECT_EQ(mir_key_input_event_modifier_shift, mir_key_input_event_get_modifiers(new_kev));
}

TEST(TouchInputEventProperties, touch_count_taken_from_pointer_count)
{
    unsigned const pointer_count = 3;

    MirEvent old_ev;
    old_ev.type = mir_event_type_motion;
    old_ev.motion.action = mir_motion_action_down;
    old_ev.motion.pointer_count = pointer_count;
    
    auto tev = mir_input_event_get_touch_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(pointer_count, mir_touch_input_event_get_touch_count(tev));
}

TEST(TouchInputEventProperties, touch_id_comes_from_pointer_coordinates)
{
    unsigned const touch_id = 31;
    MirEvent old_ev;
    old_ev.type = mir_event_type_motion;
    old_ev.motion.action = mir_motion_action_down;
    old_ev.motion.pointer_count = 1;
    old_ev.motion.pointer_coordinates[0].id = touch_id;

    auto tev = mir_input_event_get_touch_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(touch_id, mir_touch_input_event_get_touch_id(tev, 0));
}

// mir_motion_action_up/down represent the start of a gesture. pointers only go up/down one at a time
TEST(TouchInputEventProperties, down_and_up_actions_are_taken_from_old_event)
{
    MirEvent old_ev;
    old_ev.type = mir_event_type_motion;
    old_ev.motion.action = mir_motion_action_down;
    old_ev.motion.pointer_count = 1;

    auto tev = mir_input_event_get_touch_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_touch_input_event_action_down, mir_touch_input_event_get_touch_action(tev, 0));
}

TEST(TouchInputEventProperties, pointer_up_down_applies_only_to_masked_action)
{
    int const masked_pointer_index = 1;

    MirEvent old_ev;
    old_ev.type = mir_event_type_motion;
    old_ev.motion.action = masked_pointer_index << MIR_EVENT_ACTION_POINTER_INDEX_SHIFT;
    old_ev.motion.action = (old_ev.motion.action & MIR_EVENT_ACTION_POINTER_INDEX_MASK) | mir_motion_action_pointer_up;
    old_ev.motion.pointer_count = 3;

    auto tev = mir_input_event_get_touch_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_touch_input_event_action_change, mir_touch_input_event_get_touch_action(tev, 0));
    EXPECT_EQ(mir_touch_input_event_action_up, mir_touch_input_event_get_touch_action(tev, 1));
    EXPECT_EQ(mir_touch_input_event_action_change, mir_touch_input_event_get_touch_action(tev, 2));
}

TEST(TouchInputEventProperties, tool_type_copied_from_old_pc)
{
    MirEvent old_ev;
    old_ev.type = mir_event_type_motion;

    auto& old_mev = old_ev.motion;
    old_mev.pointer_count = 4;
    old_mev.pointer_coordinates[0].tool_type = mir_motion_tool_type_unknown;
    old_mev.pointer_coordinates[1].tool_type = mir_motion_tool_type_finger;
    old_mev.pointer_coordinates[2].tool_type = mir_motion_tool_type_stylus;
    old_mev.pointer_coordinates[3].tool_type = mir_motion_tool_type_mouse;

    auto tev = mir_input_event_get_touch_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_touch_input_tool_type_unknown, mir_touch_input_event_get_touch_tooltype(tev, 0));
    EXPECT_EQ(mir_touch_input_tool_type_finger, mir_touch_input_event_get_touch_tooltype(tev, 1));
    EXPECT_EQ(mir_touch_input_tool_type_stylus, mir_touch_input_event_get_touch_tooltype(tev, 2));
}

TEST(TouchInputEventProperties, axis_values_used_by_qtmir_copied)
{
    float x_value = 19, y_value = 23, touch_major = .3, touch_minor = .2, pressure = .9, size = 1111;
    MirEvent old_ev;
    old_ev.type = mir_event_type_motion;
    old_ev.motion.pointer_count = 1;
    auto &old_pc = old_ev.motion.pointer_coordinates[0];
    old_pc.x = x_value;
    old_pc.y = y_value;
    old_pc.touch_major = touch_major;
    old_pc.touch_minor = touch_minor;
    old_pc.pressure = pressure;
    old_pc.size = size;

    auto tev = mir_input_event_get_touch_input_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(x_value, mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_x));
    EXPECT_EQ(y_value, mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_y));
    EXPECT_EQ(touch_major, mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_touch_major));
    EXPECT_EQ(touch_minor, mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_touch_minor));
    EXPECT_EQ(pressure, mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_pressure));
    EXPECT_EQ(size, mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_size));
}
