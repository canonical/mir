/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/events/window_placement_event.h"

MirWindowPlacementEvent::MirWindowPlacementEvent() :
    MirEvent(mir_event_type_window_placement),
    placement_{0, 0, 0U, 0U}
{
}

auto MirWindowPlacementEvent::clone() const -> MirWindowPlacementEvent*
{
    return new MirWindowPlacementEvent{*this};
}

int MirWindowPlacementEvent::id() const
{
    return id_;
}

void MirWindowPlacementEvent::set_id(int const id)
{
    id_ = id;
}

MirRectangle MirWindowPlacementEvent::placement() const
{
    return placement_;
}

void MirWindowPlacementEvent::set_placement(MirRectangle const& placement)
{
    placement_ = placement;
}

MirWindowPlacementEvent* MirEvent::to_window_placement()
{
    return static_cast<MirWindowPlacementEvent*>(this);
}

MirWindowPlacementEvent const* MirEvent::to_window_placement() const
{
    return static_cast<MirWindowPlacementEvent const*>(this);
}
