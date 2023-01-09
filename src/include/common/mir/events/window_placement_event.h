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

#ifndef MIR_WINDOW_PLACEMENT_EVENT_H
#define MIR_WINDOW_PLACEMENT_EVENT_H

#include "mir/events/event.h"
#include "mir_toolkit/client_types.h"

struct MirWindowPlacementEvent : MirEvent
{
    MirWindowPlacementEvent();
    auto clone() const -> MirWindowPlacementEvent* override;

    int id() const;
    void set_id(int id);

    MirRectangle placement() const;
    void set_placement(MirRectangle const& placement);

private:
    int id_ = 0;
    MirRectangle placement_;
};

#endif //MIR_WINDOW_PLACEMENT_EVENT_H
