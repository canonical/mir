/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/events/input_device_state_event.h"

#include <boost/throw_exception.hpp>

MirInputDeviceStateEvent::MirInputDeviceStateEvent()
    : MirEvent(mir_event_type_input_device_state)
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
        return pointer_x;
    case mir_pointer_axis_y:
        return pointer_y;
    default:
        return 0.0f;
    }
}

void MirInputDeviceStateEvent::set_pointer_axis(MirPointerButtons axis, float value)
{
    switch(axis)
    {
    case mir_pointer_axis_x:
        pointer_x = value;
        break;
    case mir_pointer_axis_y:
        pointer_y = value;
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

void MirInputDeviceStateEvent::add_device(MirInputDeviceId id, std::vector<uint32_t> && pressed_keys, MirPointerButtons pointer_buttons)
{
    devices.emplace_back(id, std::move(pressed_keys), pointer_buttons);
}

uint32_t MirInputDeviceStateEvent::device_count() const
{
    return devices.size();
}

MirInputDeviceId MirInputDeviceStateEvent::device_id(size_t index) const
{
    return devices[index].id;
}

uint32_t const* MirInputDeviceStateEvent::device_pressed_keys(size_t index) const
{
    return devices[index].pressed_keys.data();
}

uint32_t MirInputDeviceStateEvent::device_pressed_keys_count(size_t index) const
{
    return devices[index].pressed_keys.size();
}

MirPointerButtons MirInputDeviceStateEvent::device_pointer_buttons(size_t index) const
{
    return devices[index].pointer_buttons;
}

namespace
{
void delete_input_device_state_event(MirEvent* to)
{
    delete to->to_input_device_state();
}
template<typename Data>
void encode(std::string& encoded, Data const& data)
{
    encoded.append(reinterpret_cast<char const*>(&data), sizeof data);
}
}

mir::EventUPtr MirInputDeviceStateEvent::deserialize(std::string const& bytes)
{
    auto const stream_size = bytes.size();

    MirEventType type;
    std::chrono::nanoseconds when;
    MirPointerButtons pointer_buttons;
    float pointer_x;
    float pointer_y;
    uint32_t count;

    auto minimal_event_size =
        sizeof type + sizeof when + sizeof pointer_buttons + sizeof pointer_x + sizeof pointer_y + sizeof count;

    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    char const* pos = mir::event::consume(bytes.data(), type);
    pos = mir::event::consume(pos, when);
    pos = mir::event::consume(pos, pointer_buttons);
    pos = mir::event::consume(pos, pointer_x);
    pos = mir::event::consume(pos, pointer_y);
    pos = mir::event::consume(pos, count);

    auto new_event = new MirInputDeviceStateEvent;
    new_event->set_when(when);
    new_event->set_pointer_buttons(pointer_buttons);
    new_event->set_pointer_axis(mir_pointer_axis_x, pointer_x);
    new_event->set_pointer_axis(mir_pointer_axis_y, pointer_y);

    for (size_t i = 0; i != count; ++i)
    {
        uint32_t pressed_count = 0;
        MirInputDeviceId id;
        pos = mir::event::consume(pos, id);
        pos = mir::event::consume(pos, pointer_buttons);
        pos = mir::event::consume(pos, pressed_count);

        std::vector<uint32_t> pressed_keys(pressed_count);

        for (size_t j = 0;j != pressed_count; ++j)
            pos = mir::event::consume(pos, pressed_keys[j]);

        new_event->add_device(id, std::move(pressed_keys), pointer_buttons);
    }

    return mir::EventUPtr(new_event, delete_input_device_state_event);
}

std::string MirInputDeviceStateEvent::serialize(MirEvent const* event)
{
    std::string encoded;
    auto input_state = event->to_input_device_state();
    encoded.reserve(sizeof event->type() +
                    sizeof input_state->when() +
                    sizeof input_state->pointer_buttons() +
                    sizeof input_state->pointer_axis(mir_pointer_axis_x) +
                    sizeof input_state->pointer_axis(mir_pointer_axis_y) +
                    sizeof input_state->device_count() +
                    input_state->device_count() *
                    (
                        sizeof input_state->device_id(0) +
                        sizeof input_state->device_pointer_buttons(0) +
                        sizeof input_state->device_pressed_keys_count(0)
                    )
                    );
    encode(encoded, event->type());
    encode(encoded, input_state->when());
    encode(encoded, input_state->pointer_buttons());
    encode(encoded, input_state->pointer_axis(mir_pointer_axis_x));
    encode(encoded, input_state->pointer_axis(mir_pointer_axis_y));
    encode(encoded, input_state->device_count());
    for (auto const& dev : input_state->devices)
    {
        encode(encoded, dev.id);
        encode(encoded, dev.pointer_buttons);
        encode(encoded, uint32_t(dev.pressed_keys.size()));
        for (auto const& key : dev.pressed_keys)
            encode(encoded, key);
    }

    return encoded;

}

MirEvent* MirInputDeviceStateEvent::clone() const
{
    auto new_ev = new MirInputDeviceStateEvent(*this);
    return new_ev;
}
