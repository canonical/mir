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

MirSurfaceEvent::MirSurfaceEvent() :
    MirEvent(mir_event_type_surface)
{
}

int MirSurfaceEvent::id() const
{
    return id_;
}

void MirSurfaceEvent::set_id(int id)
{
    id_ = id;
}

MirSurfaceAttrib MirSurfaceEvent::attrib() const
{
    return attrib_;
}

void MirSurfaceEvent::set_attrib(MirSurfaceAttrib attrib)
{
    attrib_ = attrib;
}

int MirSurfaceEvent::value() const
{
    return value_;
}

void MirSurfaceEvent::set_value(int value)
{
    value_ = value;
}
