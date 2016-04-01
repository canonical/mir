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

#include "mir/events/surface_event.h"

MirSurfaceEvent::MirSurfaceEvent()
{
    event.initSurface();
}

int MirSurfaceEvent::id() const
{
    return event.asReader().getSurface().getId();
}

void MirSurfaceEvent::set_id(int id)
{
    event.getSurface().setId(id);
}

MirSurfaceAttrib MirSurfaceEvent::attrib() const
{
    return static_cast<MirSurfaceAttrib>(event.asReader().getSurface().getAttrib());
}

void MirSurfaceEvent::set_attrib(MirSurfaceAttrib attrib)
{
    event.getSurface().setAttrib(static_cast<mir::capnp::SurfaceEvent::Attrib>(attrib));
}

int MirSurfaceEvent::value() const
{
    return event.asReader().getSurface().getValue();
}

void MirSurfaceEvent::set_value(int value)
{
    event.getSurface().setValue(value);
}
