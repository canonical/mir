/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/events/surface_placement_event.h"

MirSurfacePlacementEvent::MirSurfacePlacementEvent() :
    MirEvent(mir_event_type_close_surface)
{
}

MirRectangle MirSurfacePlacementEvent::placement() const
{
    return placement_;
}

void MirSurfacePlacementEvent::set_placement(MirRectangle const& placement)
{
    placement_ = placement;
}
