/*
 * Copyright © Canonical Ltd.
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

#include "keyboard_state_tracker.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>

#include <ranges>

namespace mf = mir::frontend;
namespace sr = std::ranges;

bool mf::KeyboardStateTracker::process(MirEvent const& event)
{
    if (event.type() != mir_event_type_input)
        return false;

    auto const& input_event = event.to_input();

    if (input_event->input_type() != mir_input_event_type_key)
        return false;

    auto const* key_event = input_event->to_keyboard();
    auto const keysym = key_event->keysym();
    auto const scancode = key_event->scan_code();
    auto const action = key_event->action();

    if (action == mir_keyboard_action_down)
    {
        // Set all lowercase keys to uppercase
        if (keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R)
        {
            auto const uppercase = sr::views::transform(
                pressed_keysyms,
                [](auto key)
                {
                    if (key >= XKB_KEY_a && key <= XKB_KEY_z)
                        return xkb_keysym_to_upper(key);
                    return key;
                });
            pressed_keysyms = std::unordered_set<xkb_keysym_t>(uppercase.begin(), uppercase.end());
        }

        pressed_keysyms.insert(keysym);
        pressed_scancodes.insert(scancode);
        return true;
    }
    else if (action == mir_keyboard_action_up)
    {
        // Set all uppercase keys to lowercase
        if (keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R)
        {
            auto const lowercase = sr::views::transform(
                pressed_keysyms,
                [](auto key)
                {
                    if (key >= XKB_KEY_A && key <= XKB_KEY_Z)
                        return xkb_keysym_to_lower(key);
                    return key;
                });
            pressed_keysyms = std::unordered_set<xkb_keysym_t>(lowercase.begin(), lowercase.end());
        }

        pressed_keysyms.erase(keysym);
        pressed_scancodes.erase(scancode);
        return true;
    }

    return false;
}

auto mf::KeyboardStateTracker::keysym_is_pressed(uint32_t keysym) const -> bool
{
    return pressed_keysyms.contains(keysym);
}

auto mf::KeyboardStateTracker::scancode_is_pressed(uint32_t scancode) const -> bool
{
    return pressed_scancodes.contains(scancode);
}
