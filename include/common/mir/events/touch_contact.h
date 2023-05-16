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

#ifndef MIR_EVENTS_TOUCH_CONTACT_H_
#define MIR_EVENTS_TOUCH_CONTACT_H_

#include "mir_toolkit/event.h"
#include "contact_state.h"
#include "mir/geometry/point.h"

#include <optional>

namespace mir
{
namespace events
{

using TouchContactV1 = ContactState;

struct TouchContactV2
{
    TouchContactV2()
        : touch_id{0},
          action{mir_touch_action_up},
          tooltype{mir_touch_tooltype_unknown},
          position{},
          pressure{0},
          touch_major{0},
          touch_minor{0},
          orientation{0}
    {
    }

    TouchContactV2(
        MirTouchId touch_id,
        MirTouchAction action,
        MirTouchTooltype tooltype,
        geometry::PointF position,
        float pressure,
        float touch_major,
        float touch_minor,
        float orientation)
        : touch_id{touch_id},
          action{action},
          tooltype{tooltype},
          position{position},
          pressure{pressure},
          touch_major{touch_major},
          touch_minor{touch_minor},
          orientation{orientation}
    {
    }

    TouchContactV2(
        TouchContactV1 contact)
        : touch_id{contact.touch_id},
          action{contact.action},
          tooltype{contact.tooltype},
          position{contact.x, contact.y},
          pressure{contact.pressure},
          touch_major{contact.touch_major},
          touch_minor{contact.touch_minor},
          orientation{contact.orientation}
    {
    }

    auto operator==(TouchContactV2 const& rhs) const -> bool
    {
        return
            touch_id == rhs.touch_id &&
            action == rhs.action &&
            tooltype == rhs.tooltype &&
            position == rhs.position &&
            pressure == rhs.pressure &&
            touch_major == rhs.touch_major &&
            touch_minor == rhs.touch_minor &&
            orientation == rhs.orientation;
    }

    MirTouchId touch_id;
    MirTouchAction action;
    MirTouchTooltype tooltype;
    geometry::PointF position;
    std::optional<geometry::PointF> local_position;
    float pressure;
    float touch_major;
    float touch_minor;
    float orientation;
};

using TouchContact = TouchContactV2;

}
}

#endif // MIR_EVENTS_TOUCH_CONTACT_H_
