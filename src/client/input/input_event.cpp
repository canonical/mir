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

// TODO Remove me once we use capnproto for input platform serialization
#include "mir/cookie/blob.h"

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
        case mir_input_event_type_pointer:
            return "mir_input_event_type_pointer";
        default:
            abort();
    }
}

template <typename EventType>
void expect_event_type(EventType const* ev, MirEventType t) try
{
    if (ev->type() != t)
    {
        mir::log_critical("Expected " + mir::event_type_to_string(t) + " but event is of type " +
            mir::event_type_to_string(ev->type()));
        abort();
    }
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

}

MirInputEventType mir_input_event_get_type(MirInputEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_input);

    return ev->input_type();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_input);

    return ev->device_id();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

int64_t mir_input_event_get_event_time(MirInputEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_input);

    return ev->event_time().count();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
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
    if (ev->input_type() != mir_input_event_type_key)
    {
        mir::log_critical("expected key input event but event was of type " +
            input_event_type_to_string(ev->input_type()));
        abort();
    }
    
    return reinterpret_cast<MirKeyboardEvent const*>(ev);
}

MirKeyboardAction mir_keyboard_event_action(MirKeyboardEvent const* kev) try
{
    return kev->action();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

xkb_keysym_t mir_keyboard_event_key_code(MirKeyboardEvent const* kev) try
{
    return kev->key_code();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

int mir_keyboard_event_scan_code(MirKeyboardEvent const* kev) try
{
    return kev->scan_code();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirInputEventModifiers mir_keyboard_event_modifiers(MirKeyboardEvent const* kev) try
{
    return kev->modifiers();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}
/* Touch event accessors */

MirInputEventModifiers mir_touch_event_modifiers(MirTouchEvent const* tev) try
{
    return tev->modifiers();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* ev) try
{
    if(ev->input_type() != mir_input_event_type_touch)
    {
        mir::log_critical("expected touch input event but event was of type " +
            input_event_type_to_string(ev->input_type()));
        abort();
    }

    return reinterpret_cast<MirTouchEvent const*>(ev);
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

unsigned int mir_touch_event_point_count(MirTouchEvent const* event) try
{
    return event->pointer_count();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirTouchId mir_touch_event_id(MirTouchEvent const* event, size_t touch_index) try
{
    if (touch_index >= event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    return event->id(touch_index);
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirTouchAction mir_touch_event_action(MirTouchEvent const* event, size_t touch_index) try
{
    if(touch_index > event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }
    
    return static_cast<MirTouchAction>(event->action(touch_index));
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event,
    size_t touch_index) try
{
    if(touch_index > event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    return event->tool_type(touch_index);
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

float mir_touch_event_axis_value(MirTouchEvent const* event,
    size_t touch_index, MirTouchAxis axis) try
{
    if(touch_index > event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    switch (axis)
    {
    case mir_touch_axis_x:
        return event->x(touch_index);
    case mir_touch_axis_y:
        return event->y(touch_index);
    case mir_touch_axis_pressure:
        return event->pressure(touch_index);
    case mir_touch_axis_touch_major:
        return event->touch_major(touch_index);
    case mir_touch_axis_touch_minor:
        return event->touch_minor(touch_index);
    case mir_touch_axis_size:
        return std::max(
            event->touch_major(touch_index),
            event->touch_minor(touch_index));
    default:
        return -1;
    }
}  catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

/* Pointer event accessors */

MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* ev) try
{
    if(ev->input_type() != mir_input_event_type_pointer)
    {
        mir::log_critical("expected pointer input event but event was of type " +
            input_event_type_to_string(ev->input_type()));
        abort();
    }

    return reinterpret_cast<MirPointerEvent const*>(ev);
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirInputEventModifiers mir_pointer_event_modifiers(MirPointerEvent const* pev) try
{
    return pev->modifiers();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirPointerAction mir_pointer_event_action(MirPointerEvent const* pev) try
{
    return pev->action();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

bool mir_pointer_event_button_state(MirPointerEvent const* pev,
    MirPointerButton button) try
{
   return pev->buttons() & button;
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirPointerButtons mir_pointer_event_buttons(MirPointerEvent const* pev) try
{
   return pev->buttons();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

float mir_pointer_event_axis_value(MirPointerEvent const* pev, MirPointerAxis axis) try
{
   switch (axis)
   {
   case mir_pointer_axis_x:
       return pev->x();
   case mir_pointer_axis_y:
       return pev->y();
   case mir_pointer_axis_relative_x:
       return pev->dx();
   case mir_pointer_axis_relative_y:
       return pev->dy();
   case mir_pointer_axis_vscroll:
       return pev->vscroll();
   case mir_pointer_axis_hscroll:
       return pev->hscroll();
   default:
       mir::log_critical("Invalid axis enumeration " + std::to_string(axis));
       abort();
   }
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

bool mir_input_event_has_cookie(MirInputEvent const* ev) try
{
    switch (ev->input_type())
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
        case mir_input_event_types:
            abort();
            break;
    }

    return false;
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

size_t mir_cookie_buffer_size(MirCookie const* cookie) try
{
    return cookie->size();
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirCookie const* mir_input_event_get_cookie(MirInputEvent const* iev) try
{
    if (iev->type() == mir_event_type_input)
    {
        return new MirCookie(iev->cookie());
    }
    else
    {
        mir::log_critical("expected a key or motion events, type was: " + mir::event_type_to_string(iev->type()));
        abort();
    }
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

void mir_cookie_to_buffer(MirCookie const* cookie, void* buffer, size_t size) try
{
    return cookie->copy_to(buffer, size);
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

MirCookie const* mir_cookie_from_buffer(void const* buffer, size_t size) try
{
    if (size != mir::cookie::default_blob_size)
        return NULL;

    return new MirCookie(buffer, size);
} catch (std::exception const& e)
{
    mir::log_critical(e.what());
    abort();
}

void mir_cookie_release(MirCookie const* cookie)
{
    delete cookie;
}
