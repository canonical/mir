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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/events/surface_placement_event.h"

// MirSurfacePlacementEvent is a deprecated type, but we need to implement it
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirSurfacePlacementEvent::MirSurfacePlacementEvent() : MirEvent(mir_event_type_window_placement)
{
}

auto MirSurfacePlacementEvent::clone() const -> MirSurfacePlacementEvent*
{
    return new MirSurfacePlacementEvent{*this};
}

int MirSurfacePlacementEvent::id() const
{
    return id_;
}

void MirSurfacePlacementEvent::set_id(int const id)
{
    id_ = id;
}

MirRectangle MirSurfacePlacementEvent::placement() const
{
    return placement_;
}

void MirSurfacePlacementEvent::set_placement(MirRectangle const& placement)
{
    placement_ = placement;
}

MirSurfacePlacementEvent* MirEvent::to_window_placement()
{
    return static_cast<MirSurfacePlacementEvent*>(this);
}

MirSurfacePlacementEvent const* MirEvent::to_window_placement() const
{
    return static_cast<MirSurfacePlacementEvent const*>(this);
}

