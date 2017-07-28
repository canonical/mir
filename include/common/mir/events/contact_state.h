/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Author: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_EVENTS_CONTACT_STATE_H_
#define MIR_EVENTS_CONTACT_STATE_H_

#include "mir_toolkit/event.h"

namespace mir
{
namespace events
{

struct ContactState
{
    MirTouchId touch_id;
    MirTouchAction action;
    MirTouchTooltype tooltype;
    float x;
    float y;
    float pressure;
    float touch_major;
    float touch_minor;
    float orientation;
};

inline bool operator==(ContactState const& lhs, ContactState const & rhs)
{
    return lhs.touch_id == rhs.touch_id &&
        lhs.action == rhs.action &&
        lhs.tooltype == rhs.tooltype &&
        lhs.x == rhs.x &&
        lhs.y == rhs.y &&
        lhs.pressure == rhs.pressure &&
        lhs.touch_major == rhs.touch_major &&
        lhs.touch_minor == rhs.touch_minor &&
        lhs.orientation == rhs.orientation;
}

}
}

#endif // MIR_EVENTS_CONTACT_STATE_H_
