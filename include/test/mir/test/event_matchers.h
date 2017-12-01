/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include <cmath>

#include "mir_toolkit/event.h"

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>

#include <gmock/gmock.h>


void PrintTo(MirEvent const& event, std::ostream *os);
void PrintTo(MirEvent const* event, std::ostream *os);

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

inline MirEvent const* to_address(std::shared_ptr<MirEvent const> const& event)
{
    return event.get();
}

inline MirEvent const& to_ref(MirEvent const* event)
{
    return *event;
}

inline MirEvent const& to_ref(MirEvent const& event)
{
    return event;
}

inline MirKeyboardEvent const* maybe_key_event(MirEvent const* event)
{
    if (mir_event_get_type(event) != mir_event_type_input)
        return nullptr;
    auto input_event = mir_event_get_input_event(event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
        return nullptr;
    return mir_input_event_get_keyboard_event(input_event);
}

inline MirTouchEvent const* maybe_touch_event(MirEvent const* event)
{
    if (mir_event_get_type(event) != mir_event_type_input)
        return nullptr;
    auto input_event = mir_event_get_input_event(event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_touch)
        return nullptr;
    return mir_input_event_get_touch_event(input_event);
}

inline MirPointerEvent const* maybe_pointer_event(MirEvent const* event)
{
    if (mir_event_get_type(event) != mir_event_type_input)
        return nullptr;
    auto input_event = mir_event_get_input_event(event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
        return nullptr;
    return mir_input_event_get_pointer_event(input_event);
}
/**
 * \}
 */

MATCHER(KeyDownEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
        return false;

    return true;
}

MATCHER(KeyRepeatEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if (mir_keyboard_event_action(kev) != mir_keyboard_action_repeat)
        return false;

    return true;
}

MATCHER(KeyUpEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if (mir_keyboard_event_action(kev) != mir_keyboard_action_up)
        return false;

    return true;
}

MATCHER_P(KeyWithModifiers, modifiers, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;
    
    if(mir_keyboard_event_modifiers(kev) != modifiers)
    {
        return false;
    }
    
    return true;
}

MATCHER_P(KeyOfSymbol, keysym, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;

    if(mir_keyboard_event_key_code(kev) != static_cast<xkb_keysym_t>(keysym))
        return false;

    return true;
}

MATCHER_P(KeyWithText, text, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;

    if(strcmp(mir_keyboard_event_key_text(kev), text))
        return false;

    return true;
}

MATCHER_P(KeyOfScanCode, code, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (kev == nullptr)
        return false;

    if(mir_keyboard_event_scan_code(kev) != code)
        return false;

    return true;
}

MATCHER_P(MirKeyboardEventMatches, event, "")
{
    auto expected = maybe_key_event(to_address(event));
    auto actual = maybe_key_event(to_address(arg));

    if (expected == nullptr || actual == nullptr)
        return false;

    return mir_keyboard_event_action(expected) == mir_keyboard_event_action(actual) &&
        mir_keyboard_event_key_code(expected) == mir_keyboard_event_key_code(actual) &&
        mir_keyboard_event_scan_code(expected) == mir_keyboard_event_scan_code(actual) &&
        mir_keyboard_event_modifiers(expected) == mir_keyboard_event_modifiers(actual);
}

MATCHER_P(MirTouchEventMatches, event, "")
{
    auto expected = maybe_touch_event(to_address(event));
    auto actual = maybe_touch_event(to_address(arg));
    
    if (expected == nullptr || actual == nullptr)
        return false;

    auto tc = mir_touch_event_point_count(actual);
    if (mir_touch_event_point_count(expected) != tc)
        return false;

    for (unsigned i = 0; i != tc; i++)
    {
        if (mir_touch_event_id(actual, i) !=  mir_touch_event_id(expected, i) ||
            mir_touch_event_action(actual, i) !=  mir_touch_event_action(expected, i) ||
            mir_touch_event_tooltype(actual, i) != mir_touch_event_tooltype(expected, i) ||
            mir_touch_event_axis_value(actual, i, mir_touch_axis_x) != 
                mir_touch_event_axis_value(expected, i, mir_touch_axis_x) ||
            mir_touch_event_axis_value(actual, i, mir_touch_axis_y) != 
                mir_touch_event_axis_value(expected, i, mir_touch_axis_y))
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
    if (mir_pointer_event_action(pev) == mir_pointer_action_enter)
        return true;
    return false;
}

MATCHER(PointerLeaveEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) == mir_pointer_action_leave)
        return true;
    return false;
}

inline bool button_event_matches(MirPointerEvent const* pev, float x, float y, MirPointerAction action, MirPointerButtons button_state,
                                 bool check_action = true, bool check_buttons = true, bool check_axes = true)
{
    if (pev == nullptr)
        return false;
    if (check_action && mir_pointer_event_action(pev) != action)
        return false;
    if (check_buttons && mir_pointer_event_buttons(pev) != button_state)
        return false;
    if (check_axes && mir_pointer_event_axis_value(pev, mir_pointer_axis_x) != x)
        return false;
    if (check_axes && mir_pointer_event_axis_value(pev, mir_pointer_axis_y) != y)
        return false;
    return true;
}

MATCHER_P2(ButtonDownEvent, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    return button_event_matches(pev, x, y, mir_pointer_action_button_down, 0, true, false);
}

MATCHER_P2(ButtonDownEventWithButton, pos, button, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) != mir_pointer_action_button_down)
        return false;
    if (mir_pointer_event_button_state(pev, static_cast<MirPointerButton>(button)) == false)
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_x) != pos.x.as_int())
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_y) != pos.y.as_int())
        return false;
    return true;
}

MATCHER_P2(ButtonUpEvent, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    return button_event_matches(pev, x, y, mir_pointer_action_button_up, 0, true, false);
}

MATCHER_P3(ButtonsDown, x, y, buttons, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    return button_event_matches(pev, x, y, mir_pointer_action_button_down, buttons, false);
}

MATCHER_P3(ButtonsUp, x, y, buttons, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    return button_event_matches(pev, x, y, mir_pointer_action_button_up, buttons, false);
}

MATCHER_P2(ButtonUpEventWithButton, pos, button, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) != mir_pointer_action_button_up)
        return false;
    if (mir_pointer_event_button_state(pev, button) == true)
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_x) != pos.x.as_int())
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_y) != pos.y.as_int())
        return false;
    return true;
}

MATCHER_P2(PointerAxisChange, scroll_axis, value, "")
{
    auto parg = to_address(arg);
    auto pev = maybe_pointer_event(parg);
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) != mir_pointer_action_motion)
        return false;
    if (mir_pointer_event_axis_value(pev, scroll_axis) != value)
        return false;
    return true;
}

MATCHER_P2(TouchEvent, x, y, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (tev == nullptr)
        return false;

    if (mir_touch_event_action(tev, 0) != mir_touch_action_down)
        return false;
    if (std::abs(mir_touch_event_axis_value(tev, 0, mir_touch_axis_x) - x) > 0.5f)
        return false;
    if (std::abs(mir_touch_event_axis_value(tev, 0, mir_touch_axis_y) - y) > 0.5f)
        return false;

    return true;
}

MATCHER_P4(TouchContact, slot, action, x, y, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (tev == nullptr)
        return false;

    if (mir_touch_event_action(tev, slot) != action)
        return false;
    if (std::abs(mir_touch_event_axis_value(tev, slot, mir_touch_axis_x) - x) > 0.5f)
        return false;
    if (std::abs(mir_touch_event_axis_value(tev, slot, mir_touch_axis_y) - y) > 0.5f)
        return false;

    return true;
}

MATCHER_P2(TouchUpEvent, x, y, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (tev == nullptr)
        return false;

    if (mir_touch_event_action(tev, 0) != mir_touch_action_up)
        return false;
    if (mir_touch_event_axis_value(tev, 0, mir_touch_axis_x) != x)
        return false;
    if (mir_touch_event_axis_value(tev, 0, mir_touch_axis_y) != y)
        return false;

    return true;
}

MATCHER_P2(PointerEventWithPosition, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) != mir_pointer_action_motion)
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_x) != x)
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_y) != y)
        return false;
    return true;
}

MATCHER_P2(PointerEnterEventWithPosition, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) != mir_pointer_action_enter)
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_x) != x)
        return false;
    if (mir_pointer_event_axis_value(pev, mir_pointer_axis_y) != y)
        return false;
    return true;
}


MATCHER_P(PointerEventWithModifiers, modifiers, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev && mir_pointer_event_modifiers(pev) == modifiers)
        return true;
    return false;
}

MATCHER_P2(PointerEventWithDiff, expect_dx, expect_dy, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) != mir_pointer_action_motion)
        return false;
    auto const error = 0.00001f;
    auto const actual_dx = mir_pointer_event_axis_value(pev,
                                                mir_pointer_axis_relative_x);
    if (std::abs(expect_dx - actual_dx) > error)
        return false;
    auto const actual_dy = mir_pointer_event_axis_value(pev,
                                                mir_pointer_axis_relative_y);
    if (std::abs(expect_dy - actual_dy) > error)
        return false;
    return true;
}

MATCHER_P2(PointerEnterEventWithDiff, expect_dx, expect_dy, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;
    if (mir_pointer_event_action(pev) != mir_pointer_action_enter)
        return false;
    auto const error = 0.00001f;
    auto const actual_dx = mir_pointer_event_axis_value(pev,
                                                mir_pointer_axis_relative_x);
    if (std::abs(expect_dx - actual_dx) > error)
        return false;
    auto const actual_dy = mir_pointer_event_axis_value(pev,
                                                mir_pointer_axis_relative_y);
    if (std::abs(expect_dy - actual_dy) > error)
        return false;
    return true;
}


MATCHER_P4(TouchEventInDirection, x0, y0, x1, y1, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (tev == nullptr)
        return false;

    if (mir_touch_event_action(tev, 0) != mir_touch_action_change)
        return false;

    auto x2 = mir_touch_event_axis_value(tev, 0, mir_touch_axis_x);
    auto y2 = mir_touch_event_axis_value(tev, 0, mir_touch_axis_y);

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

    if (mir_touch_event_action(tev, 0) != mir_touch_action_change)
        return false;

    return true;
}

MATCHER(PointerMovementEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (pev == nullptr)
        return false;

    if (mir_pointer_event_action(pev) != mir_pointer_action_motion)
        return false;

    return true;
}

MATCHER_P2(WindowEvent, attrib, value, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) != mir_event_type_window)
        return false;
    auto surface_ev = mir_event_get_window_event(as_address);
    auto window_attrib = mir_window_event_get_attribute(surface_ev);
    if (window_attrib != attrib)
        return false;
    if (mir_window_event_get_attribute_value(surface_ev) != value)
        return false;
    return true;
}

MATCHER_P(KeymapEventForDevice, device_id, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) != mir_event_type_keymap)
        return false;
    auto kmev = mir_event_get_keymap_event(as_address);
    return device_id == mir_keymap_event_get_device_id(kmev);
}

MATCHER_P(OrientationEvent, direction, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) != mir_event_type_orientation)
        return false;
    auto oev = mir_event_get_orientation_event(as_address);
    if (mir_orientation_event_get_direction(oev) != direction)
        return false;
    return true;
}


MATCHER_P(InputDeviceIdMatches, device_id, "")
{
    if (mir_event_get_type(to_address(arg)) != mir_event_type_input)
        return false;
    auto input_event = mir_event_get_input_event(to_address(arg));
    return mir_input_event_get_device_id(input_event) == device_id;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MATCHER(InputConfigurationEvent, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) != mir_event_type_input_configuration)
        return true;
    return false;
}
#pragma GCC diagnostic pop

MATCHER(InputDeviceStateEvent, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) == mir_event_type_input_device_state)
        return true;
    return false;
}

MATCHER_P(DeviceStateWithPressedKeys, keys, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) != mir_event_type_input_device_state)
        return false;
    auto device_state = mir_event_get_input_device_state_event(as_address);
    for (size_t index = 0, count = mir_input_device_state_event_device_count(device_state);
         index != count; ++index)
    {
        auto key_count = mir_input_device_state_event_device_pressed_keys_count(device_state, index);
        auto it_keys = begin(keys);
        auto end_keys = end(keys);
        decltype(key_count) num_required_keys = distance(it_keys, end_keys);
        if (num_required_keys != key_count)
            continue;

        std::vector<uint32_t> pressed_keys;
        for (uint32_t i = 0; i < key_count; i++)
        {
            pressed_keys.push_back(
                mir_input_device_state_event_device_pressed_keys_for_index(device_state, index, i));
        }

        if (!std::equal(it_keys, end_keys, std::begin(pressed_keys)))
            continue;
        return true;
    }
    return false;
}

MATCHER_P2(DeviceStateWithPosition, x, y, "")
{
    auto as_address = to_address(arg);
    if (mir_event_get_type(as_address) != mir_event_type_input_device_state)
        return false;
    auto device_state = mir_event_get_input_device_state_event(as_address);
    return x == mir_input_device_state_event_pointer_axis(device_state, mir_pointer_axis_x) &&
        y == mir_input_device_state_event_pointer_axis(device_state, mir_pointer_axis_y);
}

MATCHER_P(RectanglesMatches, rectangles, "")
{
    return arg == rectangles;
}

}
}

#endif
