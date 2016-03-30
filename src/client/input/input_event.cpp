/*
 * Copyright © 2014-2016 Canonical Ltd.
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

#include "mir/cookie/cookie.h"
#include "mir/event_type_to_string.h"
#include "mir/events/event_private.h"
#include "mir/log.h"
#include "mir/require.h"
#include "mir_toolkit/mir_cookie.h"

#include "../mir_cookie.h"

#include <string.h>

namespace ml = mir::logging;

namespace
{
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
    if (ev->type != mir_event_type_key && ev->type != mir_event_type_motion)
    {
        mir::log_critical("expected input event but event was of type " + mir::event_type_to_string(ev->type));
        abort();
    }

    switch (ev->type)
    {
    case mir_event_type_key:
        return mir_input_event_type_key;
    case mir_event_type_motion:
        return type_from_device_class(ev->to_motion()->source_id);
    default:
        abort();
    }
}

MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev)
{
    if(mir_event_get_type(ev) != mir_event_type_input)
    {
        mir::log_critical("expected input event but event was of type " + mir::event_type_to_string(ev->type));
        abort();
    }

    switch (ev->type)
    {
    case mir_event_type_motion:
        return ev->to_motion()->device_id;
    case mir_event_type_key:
        return ev->to_keyboard()->device_id;
    default:
        abort();
    }
}

int64_t mir_input_event_get_event_time(MirInputEvent const* ev)
{
    if(mir_event_get_type(ev) != mir_event_type_input)
    {
        mir::log_critical("expected input event but event was of type " + mir::event_type_to_string(ev->type));
        abort();
    }

    switch (ev->type)
    {
    case mir_event_type_motion:
        return ev->to_motion()->event_time.count();
    case mir_event_type_key:
        return ev->to_keyboard()->event_time.count();
    default:
        abort();
    }
}

MirInputEvent const* mir_pointer_event_input_event(MirPointerEvent const* event)
{
    return reinterpret_cast<MirInputEvent const*>(event);
}

MirInputEvent const* mir_keyboard_event_input_event(MirKeyboardEvent const* event)
{
    return reinterpret_cast<MirInputEvent const*>(event);
}

MirInputEvent const* mir_touch_event_input_event(MirTouchEvent const* event)
{
    return reinterpret_cast<MirInputEvent const*>(event);
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
    return kev->action;
}

xkb_keysym_t mir_keyboard_event_key_code(MirKeyboardEvent const* kev)
{
    return kev->key_code;
}

int mir_keyboard_event_scan_code(MirKeyboardEvent const* kev)
{
    return kev->scan_code;
}

MirInputEventModifiers mir_keyboard_event_modifiers(MirKeyboardEvent const* kev)
{    
    return kev->modifiers;
}
/* Touch event accessors */

MirInputEventModifiers mir_touch_event_modifiers(MirTouchEvent const* tev)
{    
    return tev->to_motion()->modifiers;
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
    return event->to_motion()->pointer_count;
}

MirTouchId mir_touch_event_id(MirTouchEvent const* event, size_t touch_index)
{
    if (touch_index >= event->to_motion()->pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    return event->to_motion()->pointer_coordinates[touch_index].id;
}

MirTouchAction mir_touch_event_action(MirTouchEvent const* event, size_t touch_index)
{
    if(touch_index > event->to_motion()->pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }
    
    return static_cast<MirTouchAction>(event->to_motion()->pointer_coordinates[touch_index].action);
}

MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event,
    size_t touch_index)
{
    if(touch_index > event->to_motion()->pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    return event->to_motion()->pointer_coordinates[touch_index].tool_type;
}

float mir_touch_event_axis_value(MirTouchEvent const* event,
    size_t touch_index, MirTouchAxis axis)
{
    if(touch_index > event->to_motion()->pointer_count)
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    auto const& old_pc = event->to_motion()->pointer_coordinates[touch_index];
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
    return pev->to_motion()->modifiers;
}

MirPointerAction mir_pointer_event_action(MirPointerEvent const* pev)
{    
    return static_cast<MirPointerAction>(pev->to_motion()->pointer_coordinates[0].action);
}

bool mir_pointer_event_button_state(MirPointerEvent const* pev,
    MirPointerButton button)
{
   return pev->to_motion()->buttons & button;
}

MirPointerButtons mir_pointer_event_buttons(MirPointerEvent const* pev)
{
   return pev->to_motion()->buttons;
}

float mir_pointer_event_axis_value(MirPointerEvent const* pev, MirPointerAxis axis)
{
   auto mev = pev->to_motion();
   switch (axis)
   {
   case mir_pointer_axis_x:
       return mev->pointer_coordinates[0].x;
   case mir_pointer_axis_y:
       return mev->pointer_coordinates[0].y;
   case mir_pointer_axis_relative_x:
       return mev->pointer_coordinates[0].dx;
   case mir_pointer_axis_relative_y:
       return mev->pointer_coordinates[0].dy;
   case mir_pointer_axis_vscroll:
       return mev->pointer_coordinates[0].vscroll;
   case mir_pointer_axis_hscroll:
       return mev->pointer_coordinates[0].hscroll;
   default:
       mir::log_critical("Invalid axis enumeration " + std::to_string(axis));
       abort();
   }
}

bool mir_input_event_has_cookie(MirInputEvent const* ev)
{
    switch (mir_input_event_get_type(ev))
    {
        case mir_input_event_type_key:
            return true;
        case mir_input_event_type_pointer:
        {
            auto const pev = mir_input_event_get_pointer_event(ev);
            auto const pev_action = mir_pointer_event_action(pev);
            return (pev_action == mir_pointer_action_button_up ||
                    pev_action == mir_pointer_action_button_down);
        }
        case mir_input_event_type_touch:
        {
            auto const tev = mir_input_event_get_touch_event(ev);
            auto const point_count = mir_touch_event_point_count(tev);
            for (size_t i = 0; i < point_count; i++)
            {
                auto const tev_action = mir_touch_event_action(tev, i);
                if (tev_action == mir_touch_action_up ||
                    tev_action == mir_touch_action_down)
                {
                    return true;
                }
            }
            break;
        }
    }

    return false;
}

size_t mir_cookie_buffer_size(MirCookie const* cookie) try
{
    return cookie->size();
} catch (...)
{
    abort();
}

MirCookie const* mir_input_event_get_cookie(MirInputEvent const* iev) try
{
    switch (iev->type)
    {
    case mir_event_type_motion:
        return new MirCookie(iev->to_motion()->cookie);
    case mir_event_type_key:
        return new MirCookie(iev->to_keyboard()->cookie);
    default:
    {
        mir::log_critical("expected a key or motion events, type was: " + mir::event_type_to_string(iev->type));
        abort();
    }
    }
} catch (...)
{
    abort();
}

void mir_cookie_to_buffer(MirCookie const* cookie, void* buffer, size_t size) try
{
    return cookie->copy_to(buffer, size);
} catch (...)
{
    abort();
}

MirCookie const* mir_cookie_from_buffer(void const* buffer, size_t size) try
{
    if (size != mir::cookie::default_blob_size)
        return NULL;

    return new MirCookie(buffer, size);
} catch (...)
{
    abort();
}

void mir_cookie_release(MirCookie const* cookie) try
{
    delete cookie;
} catch (...)
{
    abort();
}
