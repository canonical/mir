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
#include "mir/events/event_private.h"

#include <androidfw/Input.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>

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

MirPointerButtons mia::mir_pointer_buttons_from_android(int32_t android_state)
{
    MirPointerButtons buttons = 0;
    
    if (android_state & AMOTION_EVENT_BUTTON_PRIMARY)
        buttons |= mir_pointer_button_primary;
    if (android_state & AMOTION_EVENT_BUTTON_SECONDARY)
        buttons |= mir_pointer_button_secondary;
    if (android_state & AMOTION_EVENT_BUTTON_TERTIARY)
        buttons |= mir_pointer_button_tertiary;
    if (android_state & AMOTION_EVENT_BUTTON_BACK)
        buttons |= mir_pointer_button_back;
    if (android_state & AMOTION_EVENT_BUTTON_FORWARD)
        buttons |= mir_pointer_button_forward;

    return buttons;
}

int32_t mia::android_pointer_buttons_from_mir(MirPointerButtons buttons)
{
    int32_t android_state = 0;
    if (buttons & mir_pointer_button_primary)
        android_state |= AMOTION_EVENT_BUTTON_PRIMARY;
    if (buttons & mir_pointer_button_secondary)
        android_state |= AMOTION_EVENT_BUTTON_SECONDARY;
    if (buttons & mir_pointer_button_tertiary)
        android_state |= AMOTION_EVENT_BUTTON_TERTIARY;
    if (buttons & mir_pointer_button_back)
        android_state |= AMOTION_EVENT_BUTTON_BACK;
    if (buttons & mir_pointer_button_forward)
        android_state |= AMOTION_EVENT_BUTTON_FORWARD;
    return android_state;
}

MirTouchTooltype mia::mir_tool_type_from_android(int32_t android_tooltype)
{
    switch (android_tooltype)
    {
    case AMOTION_EVENT_TOOL_TYPE_FINGER:
        return mir_touch_tooltype_finger;
    case AMOTION_EVENT_TOOL_TYPE_STYLUS:
        return mir_touch_tooltype_stylus;
    // Pointer events do not have tool types in Mir
    case AMOTION_EVENT_TOOL_TYPE_MOUSE:
        return mir_touch_tooltype_unknown;
    case AMOTION_EVENT_TOOL_TYPE_ERASER:
        return mir_touch_tooltype_stylus;
    case AMOTION_EVENT_TOOL_TYPE_UNKNOWN:
    default:
        return mir_touch_tooltype_unknown;

    }
}

int32_t mia::android_tool_type_from_mir(MirTouchTooltype mir_tooltype)
{
    switch (mir_tooltype)
    {
    case mir_touch_tooltype_finger:
        return AMOTION_EVENT_TOOL_TYPE_FINGER;
    case mir_touch_tooltype_stylus:
        return AMOTION_EVENT_TOOL_TYPE_STYLUS;
    case mir_touch_tooltype_unknown:
    default:
        return AMOTION_EVENT_TOOL_TYPE_UNKNOWN;
    }
}

MirPointerAction mia::mir_pointer_action_from_masked_android(int32_t masked_android_action)
{
    switch (masked_android_action)
    {
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
        return mir_pointer_action_button_up;
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        return mir_pointer_action_button_down;
    case AMOTION_EVENT_ACTION_HOVER_ENTER:
        return mir_pointer_action_enter;
    case AMOTION_EVENT_ACTION_HOVER_EXIT:
        return mir_pointer_action_leave;
    default:
        return mir_pointer_action_motion;

    }
}

MirTouchAction mia::mir_touch_action_from_masked_android(int32_t masked_android_action)
{
    switch (masked_android_action)
    {
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
        return mir_touch_action_up;
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        return mir_touch_action_down;
    default:
        return mir_touch_action_change;
    }
}

bool mia::android_source_id_is_pointer_device(int32_t source_id)
{
    return source_id == AINPUT_SOURCE_MOUSE ||
        source_id == AINPUT_SOURCE_TRACKBALL ||
        source_id == AINPUT_SOURCE_TOUCHPAD;
}

int32_t mia::extract_masked_android_action_from(MirEvent const& ev)
{
    int index_with_action = -1;
    auto const& mev = ev.motion;

    for (unsigned i = 0; i < mev.pointer_count; i++)
    {
        if (mev.pointer_coordinates[i].action == mir_touch_action_change)
            continue;
        index_with_action = i;
    }
    if (index_with_action < 0)
        return AMOTION_EVENT_ACTION_MOVE;
    
    int masked_action = AMOTION_EVENT_ACTION_MASK;
    switch (mev.pointer_coordinates[index_with_action].action)
    {
    case mir_touch_action_up:
        if (mev.pointer_count != 1)
            masked_action &= AMOTION_EVENT_ACTION_POINTER_UP;
        else
            masked_action &= AMOTION_EVENT_ACTION_UP;
        break;
    case mir_touch_action_down:
        if (mev.pointer_count != 1)
            masked_action &= AMOTION_EVENT_ACTION_POINTER_DOWN;
        else
            masked_action &= AMOTION_EVENT_ACTION_DOWN;
        break;
    case mir_touch_action_change:
        masked_action &= AMOTION_EVENT_ACTION_MOVE;
    }
    if (index_with_action > 0)
        return masked_action | (index_with_action << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    return masked_action;
}

namespace
{
int count_buttons(MirPointerButtons buttons)
{
    int ret = 0;
    if (buttons & mir_pointer_button_primary) ret++;
    if (buttons & mir_pointer_button_secondary) ret++;
    if (buttons & mir_pointer_button_tertiary) ret++;
    if (buttons & mir_pointer_button_forward) ret++;
    if (buttons & mir_pointer_button_back) ret++;

    return ret;
}
}

int32_t mia::android_pointer_action_from_mir(MirPointerAction action, MirPointerButtons buttons)
{
    auto buttons_pressed = count_buttons(buttons);
    switch (action)
    {
    case mir_pointer_action_button_up:
        return buttons_pressed == 0 ? AMOTION_EVENT_ACTION_UP : AMOTION_EVENT_ACTION_POINTER_UP;
    case mir_pointer_action_button_down:
        return buttons_pressed == 1 ? AMOTION_EVENT_ACTION_DOWN : AMOTION_EVENT_ACTION_POINTER_DOWN;
    case mir_pointer_action_enter:
        return AMOTION_EVENT_ACTION_HOVER_ENTER;
    case mir_pointer_action_leave:
        return AMOTION_EVENT_ACTION_HOVER_EXIT;
    case mir_pointer_action_motion:
        return buttons_pressed ? AMOTION_EVENT_ACTION_MOVE : AMOTION_EVENT_ACTION_HOVER_MOVE;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid pointer action"));
    }
}

int32_t mia::extract_android_action_from(MirEvent const& event)
{
    if (mia::android_source_id_is_pointer_device(event.motion.source_id))
    {
        return mia::android_pointer_action_from_mir(
            static_cast<MirPointerAction>(event.motion.pointer_coordinates[0].action), event.motion.buttons);
    }
    else
    {
        return mia::extract_masked_android_action_from(event);
    }
}
