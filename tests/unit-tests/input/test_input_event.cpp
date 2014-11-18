/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include <gtest/gtest.h>

#include "mir_toolkit/event.h"
#include "mir_toolkit/input/input_event.h"

// TODO: Zero out events

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

TEST(InputEvent, motion_pointer_and_hover_actions_are_pointer_input_events_others_are_touch)
{
    MirMotionAction const actions[] = {
        mir_motion_action_down,
        mir_motion_action_up,
        mir_motion_action_move,
        mir_motion_action_cancel,
        mir_motion_action_outside,
        mir_motion_action_pointer_down,
        mir_motion_action_pointer_up,
        mir_motion_action_hover_move,
        mir_motion_action_scroll,
        mir_motion_action_hover_enter,
        mir_motion_action_hover_exit,
    };

    MirEvent motion_ev;
    motion_ev.type = mir_event_type_motion;
    
    for (auto action : actions)
    {
        motion_ev.motion.action = action;
        if (action == mir_motion_action_hover_enter ||
            action == mir_motion_action_hover_exit ||
            action == mir_motion_action_hover_move ||
            action == mir_motion_action_pointer_up ||
            action == mir_motion_action_pointer_down)
        {
            EXPECT_EQ(mir_input_event_type_pointer,
                mir_input_event_get_type(mir_event_get_input_event(&motion_ev)));
        }
        else
        {
            EXPECT_EQ(mir_input_event_type_touch,
                mir_input_event_get_type(mir_event_get_input_event(&motion_ev)));
        }
    }
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

// TODO: Key event getters
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

// TODO: Touch event getters
// TODO: Pointer event getters
