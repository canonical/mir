/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

MirKeymapEvent::MirKeymapEvent() : MirEvent{mir_event_type_keymap}
{
}

int MirKeymapEvent::surface_id() const
{
    return surface_id_;
}

auto MirKeymapEvent::clone() const -> MirKeymapEvent*
{
    return new MirKeymapEvent{*this};
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
    return buffer_.c_str();
}

void MirKeymapEvent::set_buffer(char const* buffer)
{
    buffer_ = buffer;
}

size_t MirKeymapEvent::size() const
{
    return buffer_.size();
}
