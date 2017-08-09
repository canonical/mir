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

#include "mir/events/orientation_event.h"

MirOrientationEvent::MirOrientationEvent()
{
    event.initOrientation();
}

int MirOrientationEvent::surface_id() const
{
    return event.asReader().getOrientation().getSurfaceId();
}

void MirOrientationEvent::set_surface_id(int id)
{
    event.getOrientation().setSurfaceId(id);
}

MirOrientation MirOrientationEvent::direction() const
{
    return static_cast<MirOrientation>(event.asReader().getOrientation().getDirection());
}

void MirOrientationEvent::set_direction(MirOrientation orientation)
{
    event.getOrientation().setDirection(orientation);
}
