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

#include "mir_toolkit/event.h"

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{

MATCHER(KeyDownEvent, "")
{
    if (arg->type != mir_event_type_key)
        return false;
    if (arg->key.action != mir_key_action_down) // Key down
        return false;

    return true;
}
MATCHER_P(KeyOfSymbol, keysym, "")
{
    if (static_cast<xkb_keysym_t>(arg->key.key_code) == (uint)keysym)
        return true;
    return false;
}

MATCHER(HoverEnterEvent, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_hover_enter)
        return false;

    return true;
}
MATCHER(HoverExitEvent, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_hover_exit)
        return false;

    return true;
}

MATCHER_P2(ButtonDownEvent, x, y, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_down)
        return false;
    if (arg->motion.button_state == 0)
        return false;
    if (arg->motion.pointer_coordinates[0].x != x)
        return false;
    if (arg->motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER_P2(ButtonUpEvent, x, y, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_up)
        return false;
    if (arg->motion.pointer_coordinates[0].x != x)
        return false;
    if (arg->motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER_P2(MotionEventWithPosition, x, y, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_move &&
        arg->motion.action != mir_motion_action_hover_move)
        return false;
    if (arg->motion.pointer_coordinates[0].x != x)
        return false;
    if (arg->motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER(MovementEvent, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_move &&
        arg->motion.action != mir_motion_action_hover_move)
        return false;
    return true;
}

}
}
