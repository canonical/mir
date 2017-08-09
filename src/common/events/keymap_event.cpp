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

MirKeymapEvent::MirKeymapEvent()
{
    event.initKeymap();
}

int MirKeymapEvent::surface_id() const
{
    return event.asReader().getKeymap().getSurfaceId();
}

void MirKeymapEvent::set_surface_id(int id)
{
    event.getKeymap().setSurfaceId(id);
}

MirInputDeviceId MirKeymapEvent::device_id() const
{
    return event.asReader().getKeymap().getDeviceId().getId();
}

void MirKeymapEvent::set_device_id(MirInputDeviceId id)
{
    event.getKeymap().getDeviceId().setId(id);
}

char const* MirKeymapEvent::buffer() const
{
    return event.asReader().getKeymap().getBuffer().cStr();
}

void MirKeymapEvent::set_buffer(char const* buffer)
{
    ::capnp::Text::Reader capnp_text(buffer, strlen(buffer));
    event.getKeymap().setBuffer(capnp_text);
}

size_t MirKeymapEvent::size() const
{
    return event.asReader().getKeymap().getBuffer().size();
}
