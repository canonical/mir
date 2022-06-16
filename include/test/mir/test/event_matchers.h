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
 */

#ifndef MIR_TEST_CLIENT_EVENT_MATCHERS_H_
#define MIR_TEST_CLIENT_EVENT_MATCHERS_H_

#include <cmath>

#include "mir_toolkit/event.h"

#include <boost/type_index.hpp>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>

#include <gmock/gmock.h>


void PrintTo(MirEvent const& event, std::ostream *os);
void PrintTo(MirEvent const* event, std::ostream *os);

/// Takes in two enums with get_enum_value() implemented and determines if they are the same.
/// If false, the discrepancy is logged to the MatchResultListener.
template<typename T> auto enums_match(
    T const& expected,
    T const& actual,
    testing::MatchResultListener* result_listener)
-> bool
{
    auto type_name = boost::typeindex::type_id<T>().pretty_name();
    if (expected != actual)
    {
        *result_listener << "Expected " << type_name << " (" << get_enum_value(expected) << ") does not match actual (" << get_enum_value(actual) << ")";
        return false;
    }

    return true;
}

/// Takes in two Mir*EventTypes and determines if they are the same.
/// If not, the discrepancy is logged to the MatchResultListener.
template<typename T> auto inline event_types_match(
    T const& left,
    T const& right,
    testing::MatchResultListener* result_listener)
-> bool
{
    if (left != right)
    {
        *result_listener << "Left event type (" << left << ") does not match right event type (" << right << ")";
        return false;
    }

    return true;
}

/// Takes in a Mir*EventModifiers and an int bitmask and determines if they are the same.
/// If not, the discrepancy is logged to the MatchResultListener.
template<typename T> auto inline modifiers_match(
    T const& left,
    int right,
    testing::MatchResultListener* result_listener)
-> bool
{
    if (left != right)
    {
        *result_listener << "Left modifiers (" << left << ") do not match right modifiers (" << right << ")";
        return false;
    }

    return true;
}

/// Takes in two xkb_keysym_t values and determines if they are the same.
/// If not, the discrepancy is logged to the MatchResultListener.
auto inline keysyms_match(
    xkb_keysym_t left,
    xkb_keysym_t right,
    testing::MatchResultListener* result_listener)
-> bool
{
    if (left != right)
    {
        *result_listener << "Left keysym does not match right keysym";
        return false;
    }

    return true;
}

/// Takes in two scan codes and determines if they are the same.
/// If not, the discrepancy is logged to the MatchResultListener.
auto inline scan_codes_match(
        int left,
        int right,
        testing::MatchResultListener* result_listener)
-> bool
{
    if (left != right)
    {
        *result_listener << "Left scan code (" << left << ") does not match right scan code (" << right << ")";
        return false;
    }

    return true;
}

/// Checks if a Mir*Event is a nullptr. 
/// If true, this is logged to the MatchResultListener.
template<typename T> auto inline event_is_nullptr(
    T const* event, 
    testing::MatchResultListener* result_listener)
-> bool
{
    if (event == nullptr)
    {
        *result_listener << " is nullptr";
        return true;
    }

    return false;
}

auto get_enum_value(MirEventType event)
-> char const*;
auto get_enum_value(MirInputEventType event)
-> char const*;
auto get_enum_value(MirInputEventModifier modifier)
-> char const*;
auto get_enum_value(MirKeyboardAction action) 
-> char const*;
auto get_enum_value(MirTouchAction action) 
-> char const*;
auto get_enum_value(MirTouchAxis axis)
-> char const*;
auto get_enum_value(MirTouchTooltype tooltype)
-> char const*;
auto get_enum_value(MirPointerAction action) 
-> char const*;
auto get_enum_value(MirPointerAxis axis)
-> char const*;
auto get_enum_value(MirPointerButton button)
-> char const*;
auto get_enum_value(MirPointerAxisSource source)
-> char const*;

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

inline MirEvent const* to_address(std::reference_wrapper<MirEvent> const& event)
{
    return &event.get();
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
    if (event_is_nullptr(kev, result_listener))
        return false;
    
    if (!enums_match(mir_keyboard_action_down, mir_keyboard_event_action(kev), result_listener))
        return false;

    return true;
}

MATCHER(KeyRepeatEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (event_is_nullptr(kev, result_listener))
        return false;
    
    if (!enums_match(mir_keyboard_action_repeat, mir_keyboard_event_action(kev), result_listener))
        return false;

    return true;
}

MATCHER(KeyUpEvent, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (event_is_nullptr(kev, result_listener))
        return false;
    
    if (!enums_match(mir_keyboard_action_up, mir_keyboard_event_action(kev), result_listener))
        return false;

    return true;
}

MATCHER_P(KeyWithModifiers, modifiers, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (event_is_nullptr(kev, result_listener))
        return false;
    
    if(!modifiers_match(modifiers, mir_keyboard_event_modifiers(kev), result_listener))
        return false;
    }
    
    return true;
}

MATCHER_P(KeyOfSymbol, keysym, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (event_is_nullptr(kev, result_listener))
        return false;

    if(!keysyms_match(mir_keyboard_event_keysym(kev), static_cast<xkb_keysym_t>(keysym), result_listener))
        return false;

    return true;
}

MATCHER_P(KeyWithText, text, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (event_is_nullptr(kev, result_listener))
        return false;

    if(strcmp(mir_keyboard_event_key_text(kev), text))
        return false;

    return true;
}

MATCHER_P(KeyOfScanCode, code, "")
{
    auto kev = maybe_key_event(to_address(arg));
    if (event_is_nullptr(kev, result_listener))
        return false;

    if(!scan_codes_match(mir_keyboard_event_scan_code(kev), code, result_listener))
        return false;

    return true;
}

MATCHER(KeybaordResyncEvent, "")
{
    if (!enums_match(mir_event_type_input, mir_event_get_type(arg.get()), result_listener))
    {
        return false;
    }

    return enums_match(
        mir_input_event_type_keyboard_resync,
        mir_input_event_get_type(mir_event_get_input_event(arg.get())), 
        result_listener);
}

MATCHER_P(MirKeyboardEventMatches, event, "")
{
    auto expected = maybe_key_event(to_address(event));
    auto actual = maybe_key_event(to_address(arg));

    if (event_is_nullptr(expected, result_listener) || event_is_nullptr(actual, result_listener))
        return false;

    return enums_match(mir_keyboard_event_action(expected), mir_keyboard_event_action(actual), result_listener) &&
        keysyms_match(mir_keyboard_event_keysym(expected), mir_keyboard_event_keysym(actual), result_listener) &&
        scan_codes_match(mir_keyboard_event_scan_code(expected), mir_keyboard_event_scan_code(actual), result_listener) &&
        modifiers_match(mir_keyboard_event_modifiers(expected), mir_keyboard_event_modifiers(actual), result_listener);
}

MATCHER_P(MirTouchEventMatches, event, "")
{
    auto expected = maybe_touch_event(to_address(event));
    auto actual = maybe_touch_event(to_address(arg));
    
    if (event_is_nullptr(expected, result_listener) || event_is_nullptr(actual, result_listener))
        return false;

    auto tc = mir_touch_event_point_count(actual);
    if (mir_touch_event_point_count(expected) != tc)
        return false;

    for (unsigned i = 0; i != tc; i++)
    {
            !enums_match(expected_touch_event_action, actual_touch_event_action, result_listener) ||
            !enums_match(expected_touch_tooltype, actual_touch_tooltype, result_listener) ||
        {
            return false;
        }
    }
    return true;
}

MATCHER(PointerEnterEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!enums_match(mir_pointer_action_enter, mir_pointer_event_action(pev), result_listener))
        return false;

    return true;
}

MATCHER(PointerLeaveEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!enums_match(mir_pointer_action_leave, mir_pointer_event_action(pev), result_listener))
        return false;

    return true;
}

inline bool button_event_matches(MirPointerEvent const* pev, float x, float y, MirPointerAction action, MirPointerButtons button_state,
                                 bool check_action = true, bool check_buttons = true, bool check_axes = true)
{
    if (event_is_nullptr(pev, result_listener))
        return false;
    
    if (check_action && !enums_match(mir_pointer_event_action(pev), action, result_listener))
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
    if (event_is_nullptr(pev, result_listener))
        return false;
        
    if (!actions_match(mir_pointer_event_action(pev), mir_pointer_action_button_down, result_listener))
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
    if (event_is_nullptr(pev, result_listener))
        return false;
    
    if (!enums_match(mir_pointer_action_button_up, mir_pointer_event_action(pev), result_listener))
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
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!enums_match(mir_pointer_action_motion, mir_pointer_event_action(pev), result_listener))
        return false;
    if (mir_pointer_event_axis_value(pev, scroll_axis) != value)
        return false;
    return true;
}

MATCHER_P2(TouchEvent, x, y, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (event_is_nullptr(tev, result_listener))
        return false;

    if (!enums_match(mir_touch_action_down, mir_touch_event_action(tev, 0), result_listener))
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
    if (event_is_nullptr(tev, result_listener))
        return false;

    if (!enums_match(action, mir_touch_event_action(tev, slot), result_listener))
        return false;

        return false;
    if (std::abs(mir_touch_event_axis_value(tev, slot, mir_touch_axis_y) - y) > 0.5f)
        return false;

    return true;
}

MATCHER_P2(TouchUpEvent, x, y, "")
{
    auto tev = maybe_touch_event(to_address(arg));
    if (event_is_nullptr(tev, result_listener))
        return false;

    if (!enums_match(mir_touch_action_up, mir_touch_event_action(tev, 0), result_listener))
        return false;

        return false;
    if (mir_touch_event_axis_value(tev, 0, mir_touch_axis_y) != y)
        return false;

    return true;
}

MATCHER_P2(PointerEventWithPosition, x, y, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!enums_match(mir_pointer_action_motion, mir_pointer_event_action(pev), result_listener))
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
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!enums_match(mir_pointer_action_enter, mir_pointer_event_action(pev), result_listener))
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
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!modifiers_match(mir_pointer_event_modifiers(pev), modifiers, result_listener))
        return false;

    return true;
}

MATCHER_P2(PointerEventWithDiff, expect_dx, expect_dy, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!enums_match(mir_pointer_action_motion, mir_pointer_event_action(pev), result_listener))
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
    if (event_is_nullptr(pev, result_listener))
        return false;

    if (!enums_match(mir_pointer_action_enter, mir_pointer_event_action(pev), result_listener))
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
    if (event_is_nullptr(tev, result_listener))
        return false;

    if (!enums_match(mir_touch_action_change, mir_touch_event_action(tev, 0), result_listener))
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
    if (event_is_nullptr(tev, result_listener))
        return false;

    if (!enums_match(mir_touch_action_change, mir_touch_event_action(tev, 0), result_listener))
        return false;

    return true;
}

MATCHER(PointerMovementEvent, "")
{
    auto pev = maybe_pointer_event(to_address(arg));
    if (event_is_nullptr(pev, *result_listener))
        return false;

    if (!enums_match(mir_pointer_action_motion, mir_pointer_event_action(pev), result_listener))
        return false;

    return true;
}

MATCHER_P2(WindowEvent, attrib, value, "")
{
    auto as_address = to_address(arg);
    if (!enums_match(mir_event_type_window, mir_event_get_type(as_address), result_listener))
        return false;
    
    auto surface_ev = mir_event_get_window_event(as_address);
    auto window_attrib = mir_window_event_get_attribute(surface_ev);
    if (window_attrib != attrib)
        return false;
    if (mir_window_event_get_attribute_value(surface_ev) != value)
        return false;
    return true;
}

MATCHER_P(OrientationEvent, direction, "")
{
    auto as_address = to_address(arg);
    if (!enums_match(mir_event_type_orientation, mir_event_get_type(as_address), result_listener))
        return false;
    
    if (mir_orientation_event_get_direction(oev) != direction)
        return false;
    return true;
}

MATCHER_P(InputDeviceIdMatches, device_id, "")
{
    if (!enums_match(mir_event_type_input, mir_event_get_type(to_address(arg)), result_listener))
        return false;
    
    auto input_event = mir_event_get_input_event(to_address(arg));
    return mir_input_event_get_device_id(input_event) == device_id;
}

MATCHER(InputDeviceStateEvent, "")
{
    auto as_address = to_address(arg);
    if (!enums_match(mir_event_type_input_device_state, mir_event_get_type(as_address), result_listener))
        return false;

}

MATCHER_P(DeviceStateWithPressedKeys, keys, "")
{
    auto as_address = to_address(arg);
    if (!enums_match(mir_event_type_input_device_state, mir_event_get_type(as_address), result_listener))
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
    if (!enums_match(mir_event_type_input_device_state, mir_event_get_type(as_address), result_listener))
        return false;
    
    auto device_state = mir_event_get_input_device_state_event(as_address);

}

MATCHER_P(RectanglesMatches, rectangles, "")
{
    return arg == rectangles;
}

}
}

#endif
