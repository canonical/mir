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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SURFACE_PLACEMENT_EVENT_H
#define MIR_SURFACE_PLACEMENT_EVENT_H

#include "mir/events/event.h"
#include "mir_toolkit/client_types.h"

struct MirSurfacePlacementEvent : MirEvent
{
    MirSurfacePlacementEvent();

    int id() const;
    void set_id(int id);

    MirRectangle placement() const;
    void set_placement(MirRectangle const& placement);
};

#endif //MIR_SURFACE_PLACEMENT_EVENT_H
