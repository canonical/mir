/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EVENT_FILTER_H_
#define MIR_TEST_DOUBLES_MOCK_EVENT_FILTER_H_

#include "mir/input/event_filter.h"

#include <androidfw/Input.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockEventFilter : public mir::input::EventFilter
{
    MOCK_METHOD1(handles, bool(const MirEvent&));
};
}

MATCHER_P(IsKeyEventWithKey, key, "")
{
    if (arg.type != mir_event_type_key)
        return false;

    return arg.key.key_code == key;
}
MATCHER(KeyDownEvent, "")
{
    if (arg.type != mir_event_type_key)
        return false;

    return arg.key.action == AKEY_EVENT_ACTION_DOWN;
}
MATCHER(ButtonDownEvent, "")
{
    if (arg.type != mir_event_type_motion)
        return false;
    if (arg.motion.button_state == 0)
        return false;
    return arg.motion.action == AKEY_EVENT_ACTION_DOWN;
}
MATCHER_P2(MotionEvent, dx, dy, "")
{
    if (arg.type != mir_event_type_motion)
        return false;
    auto coords = &arg.motion.pointer_coordinates[0];
    return (coords->x == dx) && (coords->y == dy);
}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_EVENT_FILTER_H_
