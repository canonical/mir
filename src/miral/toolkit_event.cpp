/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <miral/toolkit_event.h>

#include <mir_toolkit/event.h>

namespace miral
{
namespace toolkit
{
MirEventType mir_event_get_type(MirEvent const* event)
{
    return ::mir_event_get_type(event);
}

MirInputEvent const* mir_event_get_input_event(MirEvent const* event)
{
    return ::mir_event_get_input_event(event);
}

int64_t mir_input_event_get_event_time(MirInputEvent const* event)
{
    return ::mir_input_event_get_event_time(event);
}

MirInputEventType mir_input_event_get_type(MirInputEvent const* event)
{
    return ::mir_input_event_get_type(event);
}

MirKeyboardEvent const* mir_input_event_get_keyboard_event(MirInputEvent const* event)
{
    return ::mir_input_event_get_keyboard_event(event);
}

MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* event)
{
    return ::mir_input_event_get_touch_event(event);
}

MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* event)
{
    return ::mir_input_event_get_pointer_event(event);
}

bool mir_input_event_has_cookie(MirInputEvent const* ev)
{
    return ::mir_input_event_has_cookie(ev);
}

MirEvent const* mir_input_event_get_event(MirInputEvent const* event)
{
    return ::mir_input_event_get_event(event);
}

MirKeyboardAction mir_keyboard_event_action(MirKeyboardEvent const* event)
{
    return ::mir_keyboard_event_action(event);
}

xkb_keysym_t mir_keyboard_event_key_code(MirKeyboardEvent const* event)
{
    return ::mir_keyboard_event_key_code(event);
}

int mir_keyboard_event_scan_code(MirKeyboardEvent const* event)
{
    return ::mir_keyboard_event_scan_code(event);
}

char const* mir_keyboard_event_key_text(MirKeyboardEvent const* event)
{
    return ::mir_keyboard_event_key_text(event);
}

MirInputEventModifiers mir_keyboard_event_modifiers(MirKeyboardEvent const* event)
{
    return ::mir_keyboard_event_modifiers(event);
}

MirInputEvent const* mir_keyboard_event_input_event(MirKeyboardEvent const* event)
{
    return ::mir_keyboard_event_input_event(event);
}

MirInputEventModifiers mir_touch_event_modifiers(MirTouchEvent const* event)
{
    return ::mir_touch_event_modifiers(event);
}

unsigned int mir_touch_event_point_count(MirTouchEvent const* event)
{
    return ::mir_touch_event_point_count(event);
}

MirTouchId mir_touch_event_id(MirTouchEvent const* event, size_t touch_index)
{
    return ::mir_touch_event_id(event, touch_index);
}

MirTouchAction mir_touch_event_action(MirTouchEvent const* event, size_t touch_index)
{
    return ::mir_touch_event_action(event, touch_index);
}

MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event, size_t touch_index)
{
    return ::mir_touch_event_tooltype(event, touch_index);
}

float mir_touch_event_axis_value(
    MirTouchEvent const* event,
    size_t touch_index,
    MirTouchAxis axis)
{
    return ::mir_touch_event_axis_value(event, touch_index, axis);
}

MirInputEvent const* mir_touch_event_input_event(MirTouchEvent const* event)
{
    return ::mir_touch_event_input_event(event);
}

MirInputEventModifiers mir_pointer_event_modifiers(MirPointerEvent const* event)
{
    return ::mir_pointer_event_modifiers(event);
}

MirPointerAction mir_pointer_event_action(MirPointerEvent const* event)
{
    return ::mir_pointer_event_action(event);
}

bool mir_pointer_event_button_state(MirPointerEvent const* event, MirPointerButton button)
{
    return ::mir_pointer_event_button_state(event, button);
}

MirPointerButtons mir_pointer_event_buttons(MirPointerEvent const* event)
{
    return ::mir_pointer_event_buttons(event);
}

float mir_pointer_event_axis_value(MirPointerEvent const* event, MirPointerAxis axis)
{
    return ::mir_pointer_event_axis_value(event, axis);
}

MirInputEvent const* mir_pointer_event_input_event(MirPointerEvent const* event)
{
    return ::mir_pointer_event_input_event(event);
}
}
}
