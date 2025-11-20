/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_ORIENTATION_EVENT_H_
#define MIR_COMMON_ORIENTATION_EVENT_H_

#include "mir/events/event.h"

struct MirOrientationEvent : MirEvent
{
    auto clone() const -> MirOrientationEvent* override;

    MirOrientationEvent();

    int surface_id() const;
    void set_surface_id(int id);

    MirOrientation direction() const;
    void set_direction(MirOrientation orientation);

private:
    int surface_id_ = 0;
    MirOrientation direction_ = mir_orientation_normal;
};

#endif /* MIR_COMMON_ORIENTATION_EVENT_H_ */
