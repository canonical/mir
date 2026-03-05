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

#include <mir/events/keyboard_event.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <ranges>

namespace mf = mir::frontend;

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
    auto const modifiers = key_event->modifiers();

    auto const prev_shift_state = shift_state;
    shift_state = modifiers & (mir_input_event_modifier_shift | mir_input_event_modifier_shift_left |
                               mir_input_event_modifier_shift_right);

    auto processed = false;
    if (action == mir_keyboard_action_down)
    {
        pressed_keysyms.insert(keysym);
        pressed_scancodes.insert(scancode);
        processed = true;
    }
    else if (action == mir_keyboard_action_up)
    {
        pressed_keysyms.erase(keysym);
        pressed_scancodes.erase(scancode);
        processed = true;
    }

    // Transitioned from no shift to at least one shift
    if (prev_shift_state == 0 && shift_state != 0)
    {
        auto const uppercase =
            std::ranges::views::transform(pressed_keysyms, [](auto key) { return xkb_keysym_to_upper(key); });
        pressed_keysyms = std::unordered_set<xkb_keysym_t>(uppercase.begin(), uppercase.end());
    }
    else if (prev_shift_state != 0 && shift_state == 0)
    {
        // Transitioned from at least one shift to no shift
        auto const lowercase =
            std::ranges::views::transform(pressed_keysyms, [](auto key) { return xkb_keysym_to_lower(key); });
        pressed_keysyms = std::unordered_set<xkb_keysym_t>(lowercase.begin(), lowercase.end());
    }

    return processed;
}

auto mf::KeyboardStateTracker::keysym_is_pressed(uint32_t keysym) const -> bool
{
    return pressed_keysyms.contains(keysym);
}

auto mf::KeyboardStateTracker::scancode_is_pressed(uint32_t scancode) const -> bool
{
    return pressed_scancodes.contains(scancode);
}
