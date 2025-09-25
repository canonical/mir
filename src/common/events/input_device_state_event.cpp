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

#include "mir/events/input_device_state_event.h"

#include <boost/throw_exception.hpp>

MirInputDeviceStateEvent::MirInputDeviceStateEvent() : MirEvent(mir_event_type_input_device_state)
{
}

MirPointerButtons MirInputDeviceStateEvent::pointer_buttons() const
{
    return pointer_buttons_;
}

void MirInputDeviceStateEvent::set_pointer_buttons(MirPointerButtons new_pointer_buttons)
{
    pointer_buttons_ = new_pointer_buttons;
}

float MirInputDeviceStateEvent::pointer_axis(MirPointerAxis axis) const
{
    switch(axis)
    {
    case mir_pointer_axis_x:
        return pointer_axis_x;
    case mir_pointer_axis_y:
        return pointer_axis_y;
    default:
        return 0.0f;
    }
}

void MirInputDeviceStateEvent::set_pointer_axis(MirPointerButtons axis, float value)
{
    switch(axis)
    {
    case mir_pointer_axis_x:
        pointer_axis_x = value;
        break;
    case mir_pointer_axis_y:
        pointer_axis_y = value;
        break;
    default:
        break;
    }
}

std::chrono::nanoseconds MirInputDeviceStateEvent::when() const
{
    return when_;
}

void MirInputDeviceStateEvent::set_when(std::chrono::nanoseconds const& when)
{
    when_ = when;
}

MirInputEventModifiers MirInputDeviceStateEvent::modifiers() const
{
    return modifiers_;
}

void MirInputDeviceStateEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    modifiers_ = modifiers;
}

void MirInputDeviceStateEvent::set_device_states(std::vector<mir::events::InputDeviceState> const& device_states)
{
    this->device_states = device_states;
}

uint32_t MirInputDeviceStateEvent::device_count() const
{
    return device_states.size();
}

MirInputDeviceId MirInputDeviceStateEvent::device_id(size_t index) const
{
    return device_states[index].id;
}

uint32_t MirInputDeviceStateEvent::device_pressed_keys_for_index(size_t index, size_t pressed_index) const
{
    return device_states[index].pressed_keys[pressed_index];
}

uint32_t MirInputDeviceStateEvent::device_pressed_keys_count(size_t index) const
{
    return device_states[index].pressed_keys.size();
}

MirPointerButtons MirInputDeviceStateEvent::device_pointer_buttons(size_t index) const
{
    return device_states[index].buttons;
}

void MirInputDeviceStateEvent::set_window_id(int id)
{
    window_id_ = id;
}

int MirInputDeviceStateEvent::window_id() const
{
    return window_id_;
}

auto MirInputDeviceStateEvent::clone() const -> MirInputDeviceStateEvent*
{
    return new MirInputDeviceStateEvent{*this};
}
