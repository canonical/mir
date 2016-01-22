/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/events/serialization.h"
#include "mir/events/event_private.h"

#include <cstring>

namespace mev = mir::events;

namespace
{
template<typename Data>
char const* consume(char const* pos, Data& data)
{
    std::memcpy(&data, pos, sizeof data);
    return pos + sizeof data;
}

template<typename Data>
void encode(std::string& encoded, Data const& data)
{
    encoded.append(reinterpret_cast<char const*>(&data), sizeof data);
}
}

std::string mev::serialize_event(MirEvent const& event)
{
    auto type = mir_event_get_type(&event);

    switch (type)
    {
    case mir_event_type_keymap:
        {
            std::string encoded;
            auto const& keymap = event.keymap;
            encoded.reserve(sizeof event.type +
                            sizeof keymap.surface_id +
                            sizeof keymap.device_id +
                            sizeof keymap.size +
                            keymap.size);
            encode(encoded, event.type);
            encode(encoded, keymap.surface_id);
            encode(encoded, keymap.device_id);
            encode(encoded, keymap.size);
            encoded.append(keymap.buffer, keymap.size);
            return encoded;
        }
    default:
        return {reinterpret_cast<char const*>(&event), sizeof event};
    }
}

bool  mev::deserialize_event(MirEvent& event, std::string const& raw)
{
    auto minimal_event_size = sizeof event.type;
    auto const stream_size = raw.size();
    if (stream_size < minimal_event_size)
        return false;

    char const* pos = consume(raw.data(), event.type);

    switch (event.type)
    {
    case mir_event_type_keymap:
    {
        auto& keymap = event.keymap;
        minimal_event_size += sizeof keymap.surface_id + sizeof keymap.device_id + sizeof keymap.size;

        if (stream_size < minimal_event_size)
            return false;
        pos = consume(pos, keymap.surface_id);
        pos = consume(pos, keymap.device_id);
        pos = consume(pos, keymap.size);

        minimal_event_size += keymap.size;

        if (stream_size < minimal_event_size)
            return false;

        auto buffer  = static_cast<char*>(malloc(keymap.size));
        std::memcpy(buffer, pos, keymap.size);
        keymap.buffer = buffer;
        return true;
    }
    default:
        consume(raw.data(), event);
        return true;
    }
}
