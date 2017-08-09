/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/events/input_device_state_event.h"

#include <boost/throw_exception.hpp>

MirInputDeviceStateEvent::MirInputDeviceStateEvent()
{
    event.initInputDevice();
}

MirPointerButtons MirInputDeviceStateEvent::pointer_buttons() const
{
    return static_cast<MirPointerButtons>(event.asReader().getInputDevice().getButtons());
}

void MirInputDeviceStateEvent::set_pointer_buttons(MirPointerButtons new_pointer_buttons)
{
    event.getInputDevice().setButtons(new_pointer_buttons);
}

float MirInputDeviceStateEvent::pointer_axis(MirPointerAxis axis) const
{
    switch(axis)
    {
    case mir_pointer_axis_x:
        return event.asReader().getInputDevice().getPointerX();
    case mir_pointer_axis_y:
        return event.asReader().getInputDevice().getPointerY();
    default:
        return 0.0f;
    }
}

void MirInputDeviceStateEvent::set_pointer_axis(MirPointerButtons axis, float value)
{
    switch(axis)
    {
    case mir_pointer_axis_x:
        event.getInputDevice().setPointerX(value);
        break;
    case mir_pointer_axis_y:
        event.getInputDevice().setPointerY(value);
        break;
    default:
        break;
    }
}

std::chrono::nanoseconds MirInputDeviceStateEvent::when() const
{
    return std::chrono::nanoseconds{event.asReader().getInputDevice().getWhen().getCount()};
}

void MirInputDeviceStateEvent::set_when(std::chrono::nanoseconds const& when)
{
    event.getInputDevice().getWhen().setCount(when.count());
}

MirInputEventModifiers MirInputDeviceStateEvent::modifiers() const
{
    return static_cast<MirInputEventModifiers>(event.asReader().getInputDevice().getModifiers());
}

void MirInputDeviceStateEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    event.getInputDevice().setModifiers(modifiers);
}

void MirInputDeviceStateEvent::set_device_states(std::vector<mir::events::InputDeviceState> const& device_states)
{
    event.getInputDevice().initDevices(device_states.size());

    for (size_t i = 0; i < device_states.size(); i++)
    {
        auto const& state = device_states[i];
        event.getInputDevice().getDevices()[i].getDeviceId().setId(state.id);
        event.getInputDevice().getDevices()[i].setButtons(state.buttons);
        event.getInputDevice().getDevices()[i].initPressedKeys(state.pressed_keys.size());
        for (size_t j = 0; j < state.pressed_keys.size(); j++)
        {
            event.getInputDevice().getDevices()[i].getPressedKeys().set(j, state.pressed_keys[j]);
        }
    }
}

uint32_t MirInputDeviceStateEvent::device_count() const
{
    return event.asReader().getInputDevice().getDevices().size();
}

MirInputDeviceId MirInputDeviceStateEvent::device_id(size_t index) const
{
    return event.asReader().getInputDevice().getDevices()[index].getDeviceId().getId();
}

uint32_t MirInputDeviceStateEvent::device_pressed_keys_for_index(size_t index, size_t pressed_index) const
{
    return event.asReader().getInputDevice().getDevices()[index].getPressedKeys()[pressed_index];
}

uint32_t MirInputDeviceStateEvent::device_pressed_keys_count(size_t index) const
{
    return event.asReader().getInputDevice().getDevices()[index].getPressedKeys().size();
}

MirPointerButtons MirInputDeviceStateEvent::device_pointer_buttons(size_t index) const
{
    return event.asReader().getInputDevice().getDevices()[index].getButtons();
}

void MirInputDeviceStateEvent::set_window_id(int id)
{
    event.getInputDevice().setWindowId(id);
}

int MirInputDeviceStateEvent::window_id() const
{
    return event.asReader().getInputDevice().getWindowId();
}
