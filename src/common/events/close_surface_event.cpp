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

#include "mir/events/close_surface_event.h"

// MirCloseSurfaceEvent is a deprecated type, but we need to implement it
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirCloseSurfaceEvent::MirCloseSurfaceEvent()
{
    event.initCloseSurface();
}

int MirCloseSurfaceEvent::surface_id() const
{
    return event.asReader().getCloseSurface().getSurfaceId();
}

void MirCloseSurfaceEvent::set_surface_id(int id)
{
    event.getCloseSurface().setSurfaceId(id);
}
