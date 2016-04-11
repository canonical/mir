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

#include "mir/events/orientation_event.h"

MirOrientationEvent::MirOrientationEvent() :
    MirEvent(mir_event_type_orientation)
{
}

int MirOrientationEvent::surface_id() const
{
    return surface_id_;
}

void MirOrientationEvent::set_surface_id(int id)
{
    surface_id_ = id;
}

MirOrientation MirOrientationEvent::direction() const
{
    return direction_;
}

void MirOrientationEvent::set_direction(MirOrientation orientation)
{
    direction_ = orientation;
}
