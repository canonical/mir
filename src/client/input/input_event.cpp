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

#define MIR_LOG_COMPONENT "input-event-access"

#include "mir/event_type_to_string.h"
#include "mir/events/event_private.h"
#include "mir/log.h"

#include "mir_toolkit/events/input/input_event.h"

#include <assert.h>
#include <stdlib.h>

// See: https://bugs.launchpad.net/mir/+bug/1311699
#define MIR_EVENT_ACTION_MASK 0xff
#define MIR_EVENT_ACTION_POINTER_INDEX_MASK 0xff00
#define MIR_EVENT_ACTION_POINTER_INDEX_SHIFT 8;

namespace ml = mir::logging;

namespace
{
void expect_old_event_type(MirEvent const* ev, MirEventType t)
{
    if (ev->type != t)
    {
        mir::log_critical("Expected " + mir::event_type_to_string(t) + " but event is of type " +
            mir::event_type_to_string(ev->type));
        abort();
    }
}

std::string input_event_type_to_string(MirInputEventType input_event_type)
{
    switch (input_event_type)
    {
        case mir_input_event_type_key:
            return "mir_input_event_type_key";
        case mir_input_event_type_touch:
            return "mir_input_event_type_touch";
        default:
            abort();
    }
}

// Never exposed in old event, so lets avoid leaking it in to a header now.
enum 
{
    AINPUT_SOURCE_CLASS_MASK = 0x000000ff,

    AINPUT_SOURCE_CLASS_BUTTON = 0x00000001,
    AINPUT_SOURCE_CLASS_POINTER = 0x00000002,
    AINPUT_SOURCE_CLASS_NAVIGATION = 0x00000004,
    AINPUT_SOURCE_CLASS_POSITION = 0x00000008,
    AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010
};
enum 
{
    AINPUT_SOURCE_UNKNOWN = 0x00000000,

    AINPUT_SOURCE_KEYBOARD = 0x00000100 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_DPAD = 0x00000200 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_GAMEPAD = 0x00000400 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_MOUSE = 0x00002000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_STYLUS = 0x00004000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_TRACKBALL = 0x00010000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    AINPUT_SOURCE_TOUCHPAD = 0x00100000 | AINPUT_SOURCE_CLASS_POSITION,
    AINPUT_SOURCE_JOYSTICK = 0x01000000 | AINPUT_SOURCE_CLASS_JOYSTICK,

    AINPUT_SOURCE_ANY = 0xffffff00
};

MirEvent const* old_ev_from_new(MirInputEvent const* ev)
{
    return reinterpret_cast<MirEvent const*>(ev);
}

MirKeyEvent const& old_kev_from_new(MirKeyboardEvent const* ev)
{
    auto old_ev = reinterpret_cast<MirEvent const*>(ev);
    expect_old_event_type(old_ev, mir_event_type_key);
    return old_ev->key;
}

MirMotionEvent const& old_mev_from_new(MirTouchEvent const* ev)
{
    auto old_ev = reinterpret_cast<MirEvent const*>(ev);
    expect_old_event_type(old_ev, mir_event_type_motion);
    return old_ev->motion;
}

MirMotionEvent const& old_mev_from_new(MirPointerEvent const* ev)
{
    auto old_ev = reinterpret_cast<MirEvent const*>(ev);
    expect_old_event_type(old_ev, mir_event_type_motion);
    return old_ev->motion;
}

// Differentiate between MirTouchEvents and MirPointerEvents based on old device class
MirInputEventType type_from_device_class(int32_t source_class)
{
    switch (source_class)
    {
    case AINPUT_SOURCE_MOUSE:
    case AINPUT_SOURCE_TRACKBALL:
    case AINPUT_SOURCE_TOUCHPAD:
        return mir_input_event_type_pointer;
    // Realistically touch events should only come from Stylus and Touchscreen
    // device classes...practically its not clear this is a safe assumption.
    default:
        return mir_input_event_type_touch;
    }
}
}

MirInputEventType mir_input_event_get_type(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);
    
    if (old_ev->type != mir_event_type_key && old_ev->type != mir_event_type_motion)
    {
        mir::log_critical("expected input event but event was of type " + mir::event_type_to_string(old_ev->type));
        abort();
    }

    switch (old_ev->type)
    {
    case mir_event_type_key:
        return mir_input_event_type_key;
    case mir_event_type_motion:
        return type_from_device_class(old_ev->motion.source_id);
    default:
        abort();
    }
}

MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);

    if(mir_event_get_type(old_ev) != mir_event_type_input)
    {
        mir::log_critical("expected input event but event was of type " + mir::event_type_to_string(old_ev->type));
        abort();
    }

    switch (old_ev->type)
    {
    case mir_event_type_motion:
        return old_ev->motion.device_id;
    case mir_event_type_key:
        return old_ev->key.device_id;
    default:
        abort();
    }
}

int64_t mir_input_event_get_event_time(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);
    if(mir_event_get_type(old_ev) != mir_event_type_input)
    {
        mir::log_critical("expected input event but event was of type " + mir::event_type_to_string(old_ev->type));
        abort();
    }

    switch (old_ev->type)
    {
    case mir_event_type_motion:
        return old_ev->motion.event_time;
    case mir_event_type_key:
        return old_ev->key.event_time;
    default:
        abort();
    }
}

/* Key event accessors */

MirKeyboardEvent const* mir_input_event_get_keyboard_event(MirInputEvent const* ev)
{
    if (mir_input_event_get_type(ev) != mir_input_event_type_key)
    {
        mir::log_critical("expected key input event but event was of type " +
            input_event_type_to_string(mir_input_event_get_type(ev)));
        abort();
    }
    
    return reinterpret_cast<MirKeyboardEvent const*>(ev);
}

MirKeyboardAction mir_keyboard_event_action(MirKeyboardEvent const* kev)
{
    auto const& old_kev = old_kev_from_new(kev);
    
    switch (old_kev.action)
    {
    case mir_key_action_down:
        if (old_kev.repeat_count != 0)
            return mir_keyboard_action_repeat;
        else
            return mir_keyboard_action_down;
    case mir_key_action_up:
        return mir_keyboard_action_up;
    default:
        // TODO:? This means we got key_action_multiple which I dont think is 
        // actually emitted yet (and never will be as in the future it would fall under text
        // event in the new model).
        return mir_keyboard_action_down;
    }
}

xkb_keysym_t mir_keyboard_event_key_code(MirKeyboardEvent const* kev)
{
    auto const& old_kev = old_kev_from_new(kev);
    return old_kev.key_code;
}

int mir_keyboard_event_scan_code(MirKeyboardEvent const* kev)
{
    auto const& old_kev = old_kev_from_new(kev);
    return old_kev.scan_code;
}

MirInputEventModifiers mir_keyboard_event_modifiers(MirKeyboardEvent const* kev)
{    
    auto const& old_kev = old_kev_from_new(kev);
    return old_kev.modifiers;
}
/* Touch event accessors */

MirInputEventModifiers mir_touch_event_modifiers(MirTouchEvent const* tev)
{    
    auto const& old_mev = old_mev_from_new(tev);
    return old_mev.modifiers;
}

MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* ev)
{
    if(mir_input_event_get_type(ev) != mir_input_event_type_touch)
    {
        mir::log_critical("expected touch input event but event was of type " +
            input_event_type_to_string(mir_input_event_get_type(ev)));
        abort();
    }

    return reinterpret_cast<MirTouchEvent const*>(ev);
}

unsigned int mir_touch_event_point_count(MirTouchEvent const* event)
{
    auto const& old_mev = reinterpret_cast<MirEvent const*>(event)->motion;
    return old_mev.pointer_count;
}

MirTouchId mir_touch_event_id(MirTouchEvent const* event, size_t touch_index)
{
    auto const& old_mev = old_mev_from_new(event);

    if (touch_index >= old_mev.pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    return old_mev.pointer_coordinates[touch_index].id;
}

MirTouchAction mir_touch_event_action(MirTouchEvent const* event, size_t touch_index)
{
    auto const& old_mev = old_mev_from_new(event);

    if(touch_index > old_mev.pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }
    
    auto masked_action = old_mev.action & MIR_EVENT_ACTION_MASK;
    size_t masked_index = (old_mev.action & MIR_EVENT_ACTION_POINTER_INDEX_MASK) >> MIR_EVENT_ACTION_POINTER_INDEX_SHIFT;

    switch (masked_action)
    {
    // For the next two cases we could assert pc=1...because a gesture must
    // be starting or ending.
    case mir_motion_action_down:
        return mir_touch_action_down;
    case mir_motion_action_up:
        return mir_touch_action_up;
    // We can't tell which touches have actually moved without tracking state
    // so we report all touchpoints as changed.
    case mir_motion_action_move:
    case mir_motion_action_hover_move:
        return mir_touch_action_change;
    // All touch points are handled at once so we don't know the index
    case mir_motion_action_pointer_down:
        if (touch_index == masked_index)
            return mir_touch_action_down;
        else
            return mir_touch_action_change;
    case mir_motion_action_pointer_up:
        if (touch_index == masked_index)
            return mir_touch_action_up;
        else
            return mir_touch_action_change;
    // TODO: How to deal with these?
    case mir_motion_action_cancel:
    case mir_motion_action_outside:
    case mir_motion_action_scroll:
    case mir_motion_action_hover_enter:
    case mir_motion_action_hover_exit:
    default:
        return mir_touch_action_change;
    }
}

MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event,
    size_t touch_index)
{
    auto const& old_mev = old_mev_from_new(event);

    if(touch_index > old_mev.pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    switch (old_mev.pointer_coordinates[touch_index].tool_type)
    {
    case mir_motion_tool_type_finger:
        return mir_touch_tooltype_finger;
    case mir_motion_tool_type_stylus:
    case mir_motion_tool_type_eraser:
        return mir_touch_tooltype_stylus;
    case mir_motion_tool_type_mouse:
    case mir_motion_tool_type_unknown:
    default:
        return mir_touch_tooltype_unknown;
    }
}

float mir_touch_event_axis_value(MirTouchEvent const* event,
    size_t touch_index, MirTouchAxis axis)
{
    auto const& old_mev = old_mev_from_new(event);

    if(touch_index > old_mev.pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    auto const& old_pc = old_mev.pointer_coordinates[touch_index];
    switch (axis)
    {
    case mir_touch_axis_x:
        return old_pc.x;
    case mir_touch_axis_y:
        return old_pc.y;
    case mir_touch_axis_pressure:
        return old_pc.pressure;
    case mir_touch_axis_touch_major:
        return old_pc.touch_major;
    case mir_touch_axis_touch_minor:
        return old_pc.touch_minor;
    case mir_touch_axis_size:
        return old_pc.size;
    default:
        return -1;
    }
}                                                                            

/* Pointer event accessors */

MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* ev)
{
    if(mir_input_event_get_type(ev) != mir_input_event_type_pointer)
    {
        mir::log_critical("expected pointer input event but event was of type " +
            input_event_type_to_string(mir_input_event_get_type(ev)));
        abort();
    }

    return reinterpret_cast<MirPointerEvent const*>(ev);
}

MirInputEventModifiers mir_pointer_event_modifiers(MirPointerEvent const* pev)
{    
    auto const& old_mev = old_mev_from_new(pev);
    return old_mev.modifiers;
}

MirPointerAction mir_pointer_event_action(MirPointerEvent const* pev)
{    
    auto const& old_mev = old_mev_from_new(pev);
    auto masked_action = old_mev.action & MIR_EVENT_ACTION_MASK;
    switch (masked_action)
    {
    case mir_motion_action_up:
    case mir_motion_action_pointer_up:
        return mir_pointer_action_button_up;
    case mir_motion_action_down:
    case mir_motion_action_pointer_down:
        return mir_pointer_action_button_down;
    case mir_motion_action_hover_enter:
        return mir_pointer_action_enter;
    case mir_motion_action_hover_exit:
        return mir_pointer_action_leave;
    case mir_motion_action_move:
    case mir_motion_action_hover_move:
    case mir_motion_action_outside:
    default:
        return mir_pointer_action_motion;
    }
}

bool mir_pointer_event_button_state(MirPointerEvent const* pev,
    MirPointerButton button)
{
   auto const& old_mev = old_mev_from_new(pev);
   switch (button)
   {
   case mir_pointer_button_primary:
       return old_mev.button_state & mir_motion_button_primary;
   case mir_pointer_button_secondary:
       return old_mev.button_state & mir_motion_button_secondary;
   case mir_pointer_button_tertiary:
       return old_mev.button_state & mir_motion_button_tertiary;
   case mir_pointer_button_back:
       return old_mev.button_state & mir_motion_button_back;
   case mir_pointer_button_forward:
       return old_mev.button_state & mir_motion_button_forward;
   default:
       return false;
   }
}

float mir_pointer_event_axis_value(MirPointerEvent const* pev, MirPointerAxis axis)
{
   auto const& old_mev = old_mev_from_new(pev);
   switch (axis)
   {
   case mir_pointer_axis_x:
       return old_mev.pointer_coordinates[0].x;
   case mir_pointer_axis_y:
       return old_mev.pointer_coordinates[0].y;
   case mir_pointer_axis_vscroll:
       return old_mev.pointer_coordinates[0].vscroll;
   case mir_pointer_axis_hscroll:
       return old_mev.pointer_coordinates[0].hscroll;
   default:
       mir::log_critical("Invalid axis enumeration " + std::to_string(axis));
       abort();
   }
}
