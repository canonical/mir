/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_EVENT_MATCHERS_H_
#define MIR_TEST_EVENT_MATCHERS_H_

#include "mir_toolkit/event.h"

#include <androidfw/Input.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{

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

    return arg.key.action == mir_key_action_down;
}
MATCHER(ButtonDownEvent, "")
{
    if (arg.type != mir_event_type_motion)
        return false;
    if (arg.motion.button_state == 0)
        return false;
    return arg.motion.action == mir_motion_action_down;
}
MATCHER(ButtonUpEvent, "")
{
    if (arg.type != mir_event_type_motion)
        return false;
    return arg.motion.action == mir_motion_action_up;
}
MATCHER_P2(MotionEvent, dx, dy, "")
{
    if (arg.type != mir_event_type_motion)
        return false;
    auto coords = &arg.motion.pointer_coordinates[0];
    return (coords->x == dx) && (coords->y == dy);
}

MATCHER_P2(SurfaceEvent, attrib, value, "")
{
    if (arg.type != mir_event_type_surface)
        return false;
    auto surface_ev = arg.surface;
    if (surface_ev.attrib != attrib)
        return false;
    if (surface_ev.value != value)
        return false;
    return true;
}

}
} // namespace mir

#endif // MIR_TEST_EVENT_MATCHERS_H_
