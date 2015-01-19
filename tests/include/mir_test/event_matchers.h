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
inline MirEvent const* to_address(MirEvent const* event)
{
    return event;
}

inline MirEvent const* to_address(MirEvent const& event)
{
    return &event;
}

inline MirEvent const& to_ref(MirEvent const* event)
{
    return *event;
}

inline MirEvent const& to_ref(MirEvent const& event)
{
    return event;
}

inline MirKeyInputEvent const* maybe_key_event(MirEvent const* event)
{
    if (mir_event_get_type(event) != mir_event_type_input)
        return nullptr;
    auto input_event = mir_event_get_input_event(event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
        return nullptr;
    return mir_input_event_get_key_input_event(input_event);
}

inline MirTouchInputEvent const* maybe_touch_event(MirEvent const* event)
{
    if (mir_event_get_type(event) != mir_event_type_input)
        return nullptr;
    auto input_event = mir_event_get_input_event(event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_touch)
        return nullptr;
    return mir_input_event_get_touch_input_event(input_event);
}

inline MirPointerInputEvent const* maybe_pointer_event(MirEvent const* event)
{
    if (mir_event_get_type(event) != mir_event_type_input)
        return nullptr;
    auto input_event = mir_event_get_input_event(event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
        return nullptr;
    return mir_input_event_get_pointer_input_event(input_event);
}
/**
 * \}
 */

MATCHER(KeyDownEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if (mir_key_input_event_get_action(kev) != mir_key_input_event_action_down)
        return false;

    return true;
}

MATCHER(KeyRepeatEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if (mir_key_input_event_get_action(kev) != mir_key_input_event_action_repeat)
        return false;

    return true;
}

MATCHER(KeyUpEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if (mir_key_input_event_get_action(kev) != mir_key_input_event_action_up)
        return false;

    return true;
}

MATCHER_P(KeyWithModifiers, modifiers, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if(mir_key_input_event_get_modifiers(kev) != modifiers)
        {
            printf("modifiers: %d vs expected %d \n", mir_key_input_event_get_modifiers(kev), modifiers);
        return false;
        }
    
    return true;
}

MATCHER_P(KeyOfSymbol, keysym, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;

    if(mir_key_input_event_get_key_code(kev) != static_cast<xkb_keysym_t>(keysym))
        return false;

    return true;
}

MATCHER_P(MirKeyEventMatches, event, "")
{
    auto expected = maybe_key_event(to_address(event));
    auto actual = maybe_key_event(to_address(arg));
    
    if (expected == nullptr || actual == nullptr)
        return false;
    
    return  mir_key_input_event_get_action(expected) == mir_key_input_event_get_action(actual) &&
        mir_key_input_event_get_key_code(expected) == mir_key_input_event_get_key_code(actual) &&
        mir_key_input_event_get_scan_code(expected) == mir_key_input_event_get_scan_code(actual) &&
        mir_key_input_event_get_modifiers(expected) == mir_key_input_event_get_modifiers(actual);
}

MATCHER_P(MirTouchEventMatches, event, "")
{
    auto expected = maybe_touch_event(to_address(event));
    auto actual = maybe_touch_event(to_address(arg));
    
    if (expected == nullptr || actual == nullptr)
        return false;

    auto tc = mir_touch_input_event_get_touch_count(actual);
    if (mir_touch_input_event_get_touch_count(expected) != tc)
        return false;

    for (unsigned i = 0; i != tc; i++)
    {
        if (mir_touch_input_event_get_touch_id(actual, i) !=  mir_touch_input_event_get_touch_id(expected, i) ||
            mir_touch_input_event_get_touch_action(actual, i) !=  mir_touch_input_event_get_touch_action(expected, i) ||
            mir_touch_input_event_get_touch_tooltype(actual, i) != mir_touch_input_event_get_touch_tooltype(expected, i) ||
            mir_touch_input_event_get_touch_axis_value(actual, i, mir_touch_input_axis_x) != 
                mir_touch_input_event_get_touch_axis_value(expected, i, mir_touch_input_axis_x) ||
            mir_touch_input_event_get_touch_axis_value(actual, i, mir_touch_input_axis_y) != 
                mir_touch_input_event_get_touch_axis_value(expected, i, mir_touch_input_axis_y))
        {
            return false;
        }
    }
    return true;
}

MATCHER(PointerEnterEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_input_event_get_action(pev) == mir_pointer_input_event_action_enter)
        return true;
    return false;
}

MATCHER(PointerLeaveEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_input_event_get_action(pev) == mir_pointer_input_event_action_leave)
        return true;
    return false;
}

MATCHER_P2(ButtonDownEvent, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_input_event_get_action(pev) != mir_pointer_input_event_action_button_down)
        return false;
    if (mir_pointer_input_event_get_button_state(pev, mir_pointer_input_button_primary) == false)
        return false;
    if (mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_x) != x)
        return false;
    if (mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_y) != y)
        return false;
    return true;
}

MATCHER_P2(ButtonUpEvent, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_input_event_get_action(pev) != mir_pointer_input_event_action_button_up)
        return false;
    if (mir_pointer_input_event_get_button_state(pev, mir_pointer_input_button_primary) == true)
        return false;
    if (mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_x) != x)
        return false;
    if (mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_y) != y)
        return false;
    return true;
}

MATCHER_P2(TouchEvent, x, y, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (tev == nullptr)
        return false;

    if (mir_touch_input_event_get_touch_action(tev, 0) != mir_touch_input_event_action_down)
        return false;
    if (mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_x) != x)
        return false;
    if (mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_y) != y)
        return false;

    return true;
}

MATCHER_P2(PointerEventWithPosition, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_input_event_get_action(pev) != mir_pointer_input_event_action_motion)
        return false;
    if (mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_x) != x)
        return false;
    if (mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_y) != y)
        return false;
    return true;
}

MATCHER_P4(TouchEventInDirection, x0, y0, x1, y1, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (tev == nullptr)
        return false;

    if (mir_touch_input_event_get_touch_action(tev, 0) != mir_touch_input_event_action_change)
        return false;

    auto x2 = mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_x);
    auto y2 = mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_y);

    float dx1 = x1 - x0;
    float dy1 = y1 - y0;

    float dx2 = x2 - x0;
    float dy2 = y2 - y0;

    float dot_product = dx1 * dx2 + dy1 * dy2;

    // Return true if both vectors are roughly the same direction (within
    // 90 degrees).
    return dot_product > 0.0f;
}

MATCHER(TouchMovementEvent, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (tev == nullptr)
        return false;

    if (mir_touch_input_event_get_touch_action(tev, 0) != mir_touch_input_event_action_change)
        return false;

    return true;
}

MATCHER(PointerMovementEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;

    if (mir_pointer_input_event_get_action(pev) != mir_pointer_input_event_action_motion)
        return false;

    return true;
}

MATCHER_P2(SurfaceEvent, attrib, value, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) != mir_event_type_surface)
        return false;
    auto surface_ev = mir_event_get_surface_event(as_address);
    if (mir_surface_event_get_attribute(surface_ev) != attrib)
        return false;
    if (mir_surface_event_get_attribute_value(surface_ev) != value)
        return false;
    return true;
}

}
}

#endif
