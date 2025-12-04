/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/test/event_matchers.h>

#include <mir/event_printer.h>

void PrintTo(MirEvent const& event, std::ostream *os)
{
    using mir::operator<<;
    *os << event;
}

void PrintTo(MirEvent const* event, std::ostream *os)
{
    if (event)
        PrintTo(*event, os);
    else
        *os << "nullptr";
}

#define STRINGIFY(val) \
case val: \
    return #val;

auto get_enum_value(MirEventType event)
-> char const*
{
    switch(event)
    {
    STRINGIFY(mir_event_type_window)
    STRINGIFY(mir_event_type_resize)
    STRINGIFY(mir_event_type_prompt_session_state_change)
    STRINGIFY(mir_event_type_orientation)
    STRINGIFY(mir_event_type_close_window)
    STRINGIFY(mir_event_type_input)
    STRINGIFY(mir_event_type_window_output)
    STRINGIFY(mir_event_type_input_device_state)
    STRINGIFY(mir_event_type_window_placement)

    default:
        throw std::logic_error("Invalid MirEventType in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirInputEventType event)
-> char const*
{
    switch(event)
    {
    STRINGIFY(mir_input_event_type_key)
    STRINGIFY(mir_input_event_type_touch)
    STRINGIFY(mir_input_event_type_pointer)
    STRINGIFY(mir_input_event_type_keyboard_resync)

    default:
        throw std::logic_error("Invalid MirInputEventType in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirInputEventModifier modifier)
-> char const*
{
    switch(modifier)
    {
    STRINGIFY(mir_input_event_modifier_none)
    STRINGIFY(mir_input_event_modifier_alt)
    STRINGIFY(mir_input_event_modifier_alt_left)
    STRINGIFY(mir_input_event_modifier_alt_right)
    STRINGIFY(mir_input_event_modifier_shift)
    STRINGIFY(mir_input_event_modifier_shift_left)
    STRINGIFY(mir_input_event_modifier_shift_right)
    STRINGIFY(mir_input_event_modifier_sym)
    STRINGIFY(mir_input_event_modifier_function)
    STRINGIFY(mir_input_event_modifier_ctrl)
    STRINGIFY(mir_input_event_modifier_ctrl_left)
    STRINGIFY(mir_input_event_modifier_ctrl_right)
    STRINGIFY(mir_input_event_modifier_meta)
    STRINGIFY(mir_input_event_modifier_meta_left)
    STRINGIFY(mir_input_event_modifier_meta_right)
    STRINGIFY(mir_input_event_modifier_caps_lock)
    STRINGIFY(mir_input_event_modifier_num_lock)
    STRINGIFY(mir_input_event_modifier_scroll_lock)

    default:
        throw std::logic_error("Invalid MirInputEventModifier in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirKeyboardAction action)
-> char const*
{
    switch(action)
    {
    STRINGIFY(mir_keyboard_action_up)
    STRINGIFY(mir_keyboard_action_down)
    STRINGIFY(mir_keyboard_action_repeat)

    default:
        throw std::logic_error("Invalid MirKeyboardAction in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirTouchAction action)
-> char const*
{
    switch(action)
    {
    STRINGIFY(mir_touch_action_up)
    STRINGIFY(mir_touch_action_down)
    STRINGIFY(mir_touch_action_change)

    default:
        throw std::logic_error("Invalid MirTouchAction in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirTouchAxis axis)
-> char const*
{
    switch(axis)
    {
    STRINGIFY(mir_touch_axis_x)
    STRINGIFY(mir_touch_axis_y)
    STRINGIFY(mir_touch_axis_pressure)
    STRINGIFY(mir_touch_axis_touch_major)
    STRINGIFY(mir_touch_axis_touch_minor)
    STRINGIFY(mir_touch_axis_size)

    default:
        throw std::logic_error("Invalid MirTouchAxis in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirTouchTooltype tooltype)
-> char const*
{
    switch(tooltype)
    {
    STRINGIFY(mir_touch_tooltype_unknown)
    STRINGIFY(mir_touch_tooltype_finger)
    STRINGIFY(mir_touch_tooltype_stylus)

    default:
        throw std::logic_error("Invalid MirTouchTooltype in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirPointerAction action)
-> char const*
{
    switch(action)
    {
    STRINGIFY(mir_pointer_action_button_up)
    STRINGIFY(mir_pointer_action_button_down)
    STRINGIFY(mir_pointer_action_enter)
    STRINGIFY(mir_pointer_action_leave)
    STRINGIFY(mir_pointer_action_motion)

    default:
        throw std::logic_error("Invalid MirPointerAction in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirPointerAxis axis)
-> char const*
{
    switch(axis)
    {
    STRINGIFY(mir_pointer_axis_x)
    STRINGIFY(mir_pointer_axis_y)
    STRINGIFY(mir_pointer_axis_vscroll)
    STRINGIFY(mir_pointer_axis_hscroll)
    STRINGIFY(mir_pointer_axis_relative_x)
    STRINGIFY(mir_pointer_axis_relative_y)
    STRINGIFY(mir_pointer_axis_vscroll_value120)
    STRINGIFY(mir_pointer_axis_hscroll_value120)

    default:
        throw std::logic_error("Invalid MirPointerAxis in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirPointerButtons button)
-> char const*
{
    switch(button)
    {
    STRINGIFY(mir_pointer_button_primary)
    STRINGIFY(mir_pointer_button_secondary)
    STRINGIFY(mir_pointer_button_tertiary)
    STRINGIFY(mir_pointer_button_back)
    STRINGIFY(mir_pointer_button_forward)
    STRINGIFY(mir_pointer_button_side)
    STRINGIFY(mir_pointer_button_extra)
    STRINGIFY(mir_pointer_button_task)

    default:
        throw std::logic_error("Invalid MirPointerButtons in mir::test::get_enum_value()");
    }
}

auto get_enum_value(MirPointerAxisSource source)
-> char const*
{
    switch(source)
    {
    STRINGIFY(mir_pointer_axis_source_none)
    STRINGIFY(mir_pointer_axis_source_wheel)
    STRINGIFY(mir_pointer_axis_source_finger)
    STRINGIFY(mir_pointer_axis_source_continuous)
    STRINGIFY(mir_pointer_axis_source_wheel_tilt)

    default:
        throw std::logic_error("Invalid MirPointerAxisSource in mir::test::get_enum_value()");
    }
}

#undef STRINGIFY
