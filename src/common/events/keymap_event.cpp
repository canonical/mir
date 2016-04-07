/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <boost/throw_exception.hpp>

#include "mir/events/keymap_event.h"

namespace
{
void delete_keymap_event(MirEvent* e)
{
    if (e && e->to_keymap()->buffer())
    {
        e->to_keymap()->free_buffer();
    }

    delete e;
}

template<typename Data>
void encode(std::string& encoded, Data const& data)
{
    encoded.append(reinterpret_cast<char const*>(&data), sizeof data);
}
}

MirKeymapEvent::MirKeymapEvent() :
    MirEvent(mir_event_type_keymap)
{
}

MirEvent* MirKeymapEvent::clone() const
{
    auto new_ev = mir::event::deep_copy<MirKeymapEvent>(this);
    auto keymap = new_ev->to_keymap();

    // malloc to match xkbcommons allocation behavior
    auto buffer = static_cast<char*>(malloc(keymap->size()));
    std::memcpy(buffer, keymap->buffer(), keymap->size());
    keymap->set_buffer(buffer);

    return new_ev;
}

// Edge case where we have handle the char* buffer manually!
mir::EventUPtr MirKeymapEvent::deserialize(std::string const& bytes)
{
    MirInputDeviceId device_id;
    int surface_id;
    size_t size;

    auto minimal_event_size = sizeof(MirEventType);
    auto const stream_size = bytes.size();
    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    MirEventType type;
    char const* pos = mir::event::consume(bytes.data(), type);

    minimal_event_size += sizeof(surface_id) + sizeof(device_id) + sizeof(size);

    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    pos = mir::event::consume(pos, surface_id);
    pos = mir::event::consume(pos, device_id);
    pos = mir::event::consume(pos, size);

    minimal_event_size += size;

    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    auto buffer = static_cast<char*>(std::malloc(size));
    std::memcpy(buffer, pos, size);

    auto new_event = new MirKeymapEvent;
    new_event->set_surface_id(surface_id);
    new_event->set_device_id(device_id);
    new_event->set_size(size);
    new_event->set_buffer(buffer);

    return mir::EventUPtr(new_event, delete_keymap_event);
}


// Edge case where we have handle the char* buffer manually!
std::string MirKeymapEvent::serialize(MirEvent const* event)
{
    std::string encoded;
    auto keymap = event->to_keymap();
    encoded.reserve(sizeof event->type() +
                    sizeof keymap->surface_id() +
                    sizeof keymap->device_id() +
                    sizeof keymap->size() +
                    keymap->size());
    encode(encoded, event->type());
    encode(encoded, keymap->surface_id());
    encode(encoded, keymap->device_id());
    encode(encoded, keymap->size());
    encoded.append(keymap->buffer(), keymap->size());

    return encoded;
}

int MirKeymapEvent::surface_id() const
{
    return surface_id_;
}

void MirKeymapEvent::set_surface_id(int id)
{
    surface_id_ = id;
}

MirInputDeviceId MirKeymapEvent::device_id() const
{
    return device_id_;
}

void MirKeymapEvent::set_device_id(MirInputDeviceId id)
{
    device_id_ = id;
}

char const* MirKeymapEvent::buffer() const
{
    return buffer_;
}

void MirKeymapEvent::set_buffer(char const* buffer)
{
    buffer_ = buffer;
}

void MirKeymapEvent::free_buffer()
{
    std::free(const_cast<char*>(buffer_));
}

size_t MirKeymapEvent::size() const
{
    return size_;
}

void MirKeymapEvent::set_size(size_t size)
{
    size_ = size;
}
