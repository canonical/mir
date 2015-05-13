/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/input/android/event_conversion_helpers.h"

#include <androidfw/Input.h>

namespace mia = mir::input::android;

MirInputEventModifiers mia::mir_modifiers_from_android(int32_t android_modifiers)
{
    MirInputEventModifiers ret = 0;
    if (android_modifiers == 0) ret |= mir_input_event_modifier_none;
    if (android_modifiers & AMETA_ALT_ON) ret |= mir_input_event_modifier_alt;
    if (android_modifiers & AMETA_ALT_LEFT_ON) ret |= mir_input_event_modifier_alt_left;
    if (android_modifiers & AMETA_ALT_RIGHT_ON) ret |= mir_input_event_modifier_alt_right;
    if (android_modifiers & AMETA_SHIFT_ON) ret |= mir_input_event_modifier_shift;
    if (android_modifiers & AMETA_SHIFT_LEFT_ON) ret |= mir_input_event_modifier_shift_left;
    if (android_modifiers & AMETA_SHIFT_RIGHT_ON) ret |= mir_input_event_modifier_shift_right;
    if (android_modifiers & AMETA_SYM_ON) ret |= mir_input_event_modifier_sym;
    if (android_modifiers & AMETA_FUNCTION_ON) ret |= mir_input_event_modifier_function;
    if (android_modifiers & AMETA_CTRL_ON) ret |= mir_input_event_modifier_ctrl;
    if (android_modifiers & AMETA_CTRL_LEFT_ON) ret |= mir_input_event_modifier_ctrl_left;
    if (android_modifiers & AMETA_CTRL_RIGHT_ON) ret |= mir_input_event_modifier_ctrl_right;
    if (android_modifiers & AMETA_META_ON) ret |= mir_input_event_modifier_meta;
    if (android_modifiers & AMETA_META_LEFT_ON) ret |= mir_input_event_modifier_meta_left;
    if (android_modifiers & AMETA_META_RIGHT_ON) ret |= mir_input_event_modifier_meta_right;
    if (android_modifiers & AMETA_CAPS_LOCK_ON) ret |= mir_input_event_modifier_caps_lock;
    if (android_modifiers & AMETA_NUM_LOCK_ON) ret |= mir_input_event_modifier_num_lock;
    if (android_modifiers & AMETA_SCROLL_LOCK_ON) ret |= mir_input_event_modifier_scroll_lock;
    return ret;
}

int32_t mia::android_modifiers_from_mir(MirInputEventModifiers mir_modifiers)
{
    int32_t ret = AMETA_NONE;
    if (mir_modifiers & mir_input_event_modifier_alt) ret |= AMETA_ALT_ON;
    if (mir_modifiers & mir_input_event_modifier_alt_left) ret |= AMETA_ALT_LEFT_ON;
    if (mir_modifiers & mir_input_event_modifier_alt_right) ret |= AMETA_ALT_RIGHT_ON;
    if (mir_modifiers & mir_input_event_modifier_shift) ret |= AMETA_SHIFT_ON;
    if (mir_modifiers & mir_input_event_modifier_shift_left) ret |= AMETA_SHIFT_LEFT_ON;
    if (mir_modifiers & mir_input_event_modifier_shift_right) ret |= AMETA_SHIFT_RIGHT_ON;
    if (mir_modifiers & mir_input_event_modifier_sym) ret |= AMETA_SYM_ON;
    if (mir_modifiers & mir_input_event_modifier_function) ret |= AMETA_FUNCTION_ON;
    if (mir_modifiers & mir_input_event_modifier_ctrl) ret |= AMETA_CTRL_ON;
    if (mir_modifiers & mir_input_event_modifier_ctrl_left) ret |= AMETA_CTRL_LEFT_ON;
    if (mir_modifiers & mir_input_event_modifier_ctrl_right) ret |= AMETA_CTRL_RIGHT_ON;
    if (mir_modifiers & mir_input_event_modifier_meta) ret |= AMETA_META_ON;
    if (mir_modifiers & mir_input_event_modifier_meta_left) ret |= AMETA_META_LEFT_ON;
    if (mir_modifiers & mir_input_event_modifier_meta_right) ret |= AMETA_META_RIGHT_ON;
    if (mir_modifiers & mir_input_event_modifier_caps_lock) ret |= AMETA_CAPS_LOCK_ON;
    if (mir_modifiers & mir_input_event_modifier_num_lock) ret |= AMETA_NUM_LOCK_ON;
    if (mir_modifiers & mir_input_event_modifier_scroll_lock) ret |= AMETA_SCROLL_LOCK_ON;
    return ret;
}

MirKeyboardAction mia::mir_keyboard_action_from_android(int32_t android_action, int32_t repeat_count)
{
    if (repeat_count > 0)
        return mir_keyboard_action_repeat;
    
    switch (android_action)
    {
    case AKEY_EVENT_ACTION_DOWN:
    case AKEY_EVENT_ACTION_MULTIPLE:
        return mir_keyboard_action_down;
    case AKEY_EVENT_ACTION_UP:
        return mir_keyboard_action_up;
    default:
        return mir_keyboard_action_down;
    }
}

int32_t mia::android_keyboard_action_from_mir(int32_t& repeat_count_out, MirKeyboardAction action)
{
    repeat_count_out = 0;
    switch (action)
    {
    case mir_keyboard_action_repeat:
        repeat_count_out = 1;
    case mir_keyboard_action_down:
        return AKEY_EVENT_ACTION_DOWN;
    case mir_keyboard_action_up:
    default:
        return AKEY_EVENT_ACTION_UP;
    }
}
