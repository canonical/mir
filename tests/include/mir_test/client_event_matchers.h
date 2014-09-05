/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_CLIENT_EVENT_MATCHERS_H_
#define MIR_TEST_CLIENT_EVENT_MATCHERS_H_

#include "mir_toolkit/event.h"

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
/*!
 * Pointer and reference adaptors for MirEvent inside gmock matchers.
 * \{
 */
inline MirEvent const& to_ref(MirEvent const* event)
{
    return *event;
}

inline MirEvent const& to_ref(MirEvent const& event)
{
    return event;
}
/**
 * \}
 */

MATCHER(KeyDownEvent, "")
{
    if (to_ref(arg).type != mir_event_type_key)
        return false;
    if (to_ref(arg).key.action != mir_key_action_down) // Key down
        return false;

    return true;
}
MATCHER(KeyUpEvent, "")
{
    if (to_ref(arg).type != mir_event_type_key)
        return false;

    return to_ref(arg).key.action == mir_key_action_up;
}

MATCHER_P(KeyWithFlag, flag, "")
{
    if (to_ref(arg).type != mir_event_type_key)
        return false;

    return to_ref(arg).key.flags == flag;
}

MATCHER_P(KeyWithModifiers, modifiers, "")
{
    if (to_ref(arg).type != mir_event_type_key)
        return false;

    return to_ref(arg).key.modifiers == modifiers;
}

MATCHER_P(KeyOfSymbol, keysym, "")
{
    if (static_cast<xkb_keysym_t>(to_ref(arg).key.key_code) == static_cast<uint32_t>(keysym))
        return true;
    return false;
}

MATCHER_P(MirKeyEventMatches, event, "")
{
    MirEvent const& expected = to_ref(event);
    MirEvent const& actual = to_ref(arg);
    return expected.type == actual.type &&
           expected.key.device_id == actual.key.device_id &&
           expected.key.source_id == actual.key.source_id &&
           expected.key.action == actual.key.action &&
           expected.key.flags == actual.key.flags &&
           expected.key.modifiers == actual.key.modifiers &&
           expected.key.key_code == actual.key.key_code &&
           expected.key.scan_code == actual.key.scan_code &&
           expected.key.repeat_count == actual.key.repeat_count &&
           expected.key.down_time == actual.key.down_time &&
           expected.key.event_time == actual.key.event_time &&
           expected.key.is_system_key == actual.key.is_system_key;
}

MATCHER_P(MirMotionEventMatches, event, "")
{
    MirEvent const& expected = to_ref(event);
    MirEvent const& actual = to_ref(arg);
    bool scalar_members_match =
        expected.type == actual.type &&
        expected.motion.device_id == actual.motion.device_id &&
        expected.motion.source_id == actual.motion.source_id &&
        expected.motion.action == actual.motion.action &&
        expected.motion.flags == actual.motion.flags &&
        expected.motion.modifiers == actual.motion.modifiers &&
        expected.motion.edge_flags == actual.motion.edge_flags &&
        expected.motion.button_state == actual.motion.button_state &&
        expected.motion.x_offset == actual.motion.x_offset &&
        expected.motion.y_offset == actual.motion.y_offset &&
        expected.motion.x_precision == actual.motion.x_precision &&
        expected.motion.y_precision == actual.motion.y_precision &&
        expected.motion.event_time == actual.motion.event_time &&
        expected.motion.down_time == actual.motion.down_time &&
        expected.motion.pointer_count == actual.motion.pointer_count;

    if (!scalar_members_match)
        return false;

    for (size_t i = 0; i != actual.motion.pointer_count; ++i)
    {
        auto const& expected_coord = expected.motion.pointer_coordinates[i];
        auto const& actual_coord = actual.motion.pointer_coordinates[i];
        bool same_coord =
           expected_coord.id == actual_coord.id &&
           expected_coord.x == actual_coord.x &&
           expected_coord.y == actual_coord.y &&
           expected_coord.raw_x == actual_coord.raw_x &&
           expected_coord.raw_y == actual_coord.raw_y &&
           expected_coord.touch_major == actual_coord.touch_major &&
           expected_coord.touch_minor == actual_coord.touch_minor &&
           expected_coord.size == actual_coord.size &&
           expected_coord.pressure == actual_coord.pressure &&
           expected_coord.orientation == actual_coord.orientation &&
           expected_coord.vscroll == actual_coord.vscroll &&
           expected_coord.hscroll == actual_coord.hscroll &&
           expected_coord.tool_type == actual_coord.tool_type;
        if (!same_coord)
            return false;
    }
    return true;
}

MATCHER(HoverEnterEvent, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_hover_enter)
        return false;

    return true;
}
MATCHER(HoverExitEvent, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_hover_exit)
        return false;

    return true;
}

MATCHER_P2(ButtonDownEvent, x, y, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_down)
        return false;
    if (to_ref(arg).motion.button_state == 0)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].x != x)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER_P2(ButtonUpEvent, x, y, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_up)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].x != x)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER_P2(TouchEvent, x, y, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_down)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].x != x)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER_P2(MotionEventWithPosition, x, y, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_move &&
        to_ref(arg).motion.action != mir_motion_action_hover_move)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].x != x)
        return false;
    if (to_ref(arg).motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER_P4(MotionEventInDirection, x0, y0, x1, y1, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_move &&
        to_ref(arg).motion.action != mir_motion_action_hover_move)
        return false;

    auto x2 = to_ref(arg).motion.pointer_coordinates[0].x;
    auto y2 = to_ref(arg).motion.pointer_coordinates[0].y;

    float dx1 = x1 - x0;
    float dy1 = y1 - y0;

    float dx2 = x2 - x0;
    float dy2 = y2 - y0;

    float dot_product = dx1 * dx2 + dy1 + dy2;

    // Return true if both vectors are roughly the same direction (within
    // 90 degrees).
    return dot_product > 0.0f;
}

MATCHER(MovementEvent, "")
{
    if (to_ref(arg).type != mir_event_type_motion)
        return false;
    if (to_ref(arg).motion.action != mir_motion_action_move &&
        to_ref(arg).motion.action != mir_motion_action_hover_move)
        return false;
    return true;
}

}
}

#endif
