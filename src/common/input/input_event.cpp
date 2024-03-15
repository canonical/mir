/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define MIR_LOG_COMPONENT "input-event-access"


#include "mir/events/event_type_to_string.h"
#include "mir/events/event_private.h"
#include "mir/log.h"

#include "../handle_event_exception.h"

#include <string.h>
#include <mir_toolkit/events/input/pointer_event.h>


namespace ml = mir::logging;
namespace geom = mir::geometry;


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
void expect_event_type(EventType const* ev, MirEventType t) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (ev->type() != t)
    {
        mir::log_critical("Expected " + mir::event_type_to_string(t) + " but event is of type " +
            mir::event_type_to_string(ev->type()));
        abort();
    }
})
}

MirInputEventType mir_input_event_get_type(MirInputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input);

    return ev->input_type();
})

MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input);

    return ev->device_id();
})

int64_t mir_input_event_get_event_time(MirInputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input);

    return ev->event_time().count();
})

uint32_t mir_input_event_get_wayland_timestamp(MirInputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input);

    auto const ns = ev->event_time();
    auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    return ms.count();
})

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

MirKeyboardAction mir_keyboard_event_action(MirKeyboardEvent const* kev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return kev->action();
})

xkb_keysym_t mir_keyboard_event_keysym(MirKeyboardEvent const* kev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return kev->keysym();
})

int mir_keyboard_event_scan_code(MirKeyboardEvent const* kev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return kev->scan_code();
})

MirInputEventModifiers mir_keyboard_event_modifiers(MirKeyboardEvent const* kev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return kev->modifiers();
})

char const* mir_keyboard_event_key_text(MirKeyboardEvent const* kev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return kev->text();
})

/* Touch event accessors */

MirInputEventModifiers mir_touch_event_modifiers(MirTouchEvent const* tev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return tev->modifiers();
})

MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    if(ev->input_type() != mir_input_event_type_touch)
    {
        mir::log_critical("expected touch input event but event was of type " +
            input_event_type_to_string(ev->input_type()));
        abort();
    }

    return reinterpret_cast<MirTouchEvent const*>(ev);
})

unsigned int mir_touch_event_point_count(MirTouchEvent const* event) MIR_HANDLE_EVENT_EXCEPTION(
{
    return event->pointer_count();
})

MirTouchId mir_touch_event_id(MirTouchEvent const* event, size_t touch_index) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (touch_index >= event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }
    return event->id(touch_index);

})

MirTouchAction mir_touch_event_action(MirTouchEvent const* event, size_t touch_index) MIR_HANDLE_EVENT_EXCEPTION(
{
    if(touch_index > event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    return static_cast<MirTouchAction>(event->action(touch_index));
})

MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event,
    size_t touch_index) MIR_HANDLE_EVENT_EXCEPTION(
{
    if(touch_index > event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    return event->tool_type(touch_index);
})

float mir_touch_event_axis_value(MirTouchEvent const* event,
    size_t touch_index, MirTouchAxis axis) MIR_HANDLE_EVENT_EXCEPTION(
{
    if(touch_index > event->pointer_count())
    {
        mir::log_critical("touch index is greater than pointer count");
        abort();
    }

    switch (axis)
    {
    case mir_touch_axis_x:
        return event->position(touch_index).x.as_value();
    case mir_touch_axis_y:
        return event->position(touch_index).y.as_value();
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
})

/* Pointer event accessors */

MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    if(ev->input_type() != mir_input_event_type_pointer)
    {
        mir::log_critical("expected pointer input event but event was of type " +
            input_event_type_to_string(ev->input_type()));
        abort();
    }

    return reinterpret_cast<MirPointerEvent const*>(ev);
})

MirEvent const* mir_input_event_get_event(MirInputEvent const* event)
{
    return event;
}


MirPointerAxisSource mir_pointer_event_axis_source(MirPointerEvent const* event) MIR_HANDLE_EVENT_EXCEPTION(
{
    return event->axis_source();
})

MirInputEventModifiers mir_pointer_event_modifiers(MirPointerEvent const* pev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return pev->modifiers();
})

MirPointerAction mir_pointer_event_action(MirPointerEvent const* pev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return pev->action();
})

bool mir_pointer_event_button_state(MirPointerEvent const* pev,
    MirPointerButton button) MIR_HANDLE_EVENT_EXCEPTION(
{
   return pev->buttons() & button;
})

MirPointerButtons mir_pointer_event_buttons(MirPointerEvent const* pev) MIR_HANDLE_EVENT_EXCEPTION(
{
   return pev->buttons();
})

float mir_pointer_event_axis_value(MirPointerEvent const* pev, MirPointerAxis axis) MIR_HANDLE_EVENT_EXCEPTION(
{
   switch (axis)
   {
   case mir_pointer_axis_x:
       return pev->position().value_or(geom::PointF{}).x.as_value();
   case mir_pointer_axis_y:
       return pev->position().value_or(geom::PointF{}).y.as_value();
   case mir_pointer_axis_relative_x:
       return pev->motion().dx.as_value();
   case mir_pointer_axis_relative_y:
       return pev->motion().dy.as_value();
   case mir_pointer_axis_vscroll:
       return pev->v_scroll().precise.as_value();
   case mir_pointer_axis_hscroll:
       return pev->h_scroll().precise.as_value();
   case mir_pointer_axis_vscroll_discrete:
       return pev->v_scroll().discrete.as_value();
   case mir_pointer_axis_hscroll_discrete:
       return pev->h_scroll().discrete.as_value();
   case mir_pointer_axis_vscroll_value120:
       return pev->v_scroll().value120.as_value();
   case mir_pointer_axis_hscroll_value120:
       return pev->h_scroll().value120.as_value();
   default:
       mir::log_critical("Invalid axis enumeration " + std::to_string(axis));
       abort();
   }
})

bool mir_pointer_event_axis_stop(MirPointerEvent const* pev, MirPointerAxis axis) MIR_HANDLE_EVENT_EXCEPTION(
{
   switch (axis)
   {
   case mir_pointer_axis_vscroll:
       return pev->v_scroll().stop;
   case mir_pointer_axis_hscroll:
       return pev->h_scroll().stop;
   case mir_pointer_axis_x:
   case mir_pointer_axis_y:
   case mir_pointer_axis_relative_x:
   case mir_pointer_axis_relative_y:
   case mir_pointer_axis_vscroll_discrete:
   case mir_pointer_axis_hscroll_discrete:
   case mir_pointer_axis_vscroll_value120:
   case mir_pointer_axis_hscroll_value120:
       return false;
   default:
       mir::log_critical("Invalid axis enumeration " + std::to_string(axis));
       abort();
   }
})