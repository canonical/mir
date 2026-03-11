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
#include "mir_toolkit/events/enums.h"

#include <mir/events/keyboard_event.h>
#include <mir/fatal.h>

#include <xkbcommon/xkbcommon.h>

#include <algorithm>

namespace mf = mir::frontend;

namespace
{
// xkb scancodes are offset by 8 from evdev scancodes for compatibility with
// the X protocol. This matches the same helper used in xkb_mapper.cpp.
uint32_t constexpr to_xkb_scan_code(uint32_t evdev_scan_code)
{
    return evdev_scan_code + 8;
}
} // namespace

void mf::KeyboardStateTracker::XkbKeyState::update_keymap(
    std::shared_ptr<mir::input::Keymap> const& new_keymap, xkb_context* context)
{
    if (!new_keymap)
        mir::fatal_error("KeyboardStateTracker: received null keymap");

    if (current_keymap && current_keymap->matches(*new_keymap))
        return;

    current_keymap = new_keymap;
    compiled_keymap = new_keymap->make_unique_xkb_keymap(context);
    state = {xkb_state_new(compiled_keymap.get()), xkb_state_unref};
}

void mf::KeyboardStateTracker::XkbKeyState::update_key(uint32_t xkb_keycode, MirKeyboardAction action)
{
    xkb_state_update_key(state.get(), xkb_keycode, action == mir_keyboard_action_down ? XKB_KEY_DOWN : XKB_KEY_UP);
}

void mf::KeyboardStateTracker::XkbKeyState::rederive_keysyms_from_scancodes(
    std::unordered_map<uint32_t, xkb_keysym_t>& scancode_to_keysym) const
{

    for (auto& [sc, ks] : scancode_to_keysym)
    {
        auto const derived = xkb_state_key_get_one_sym(state.get(), to_xkb_scan_code(sc));
        if (derived != XKB_KEY_NoSymbol)
            ks = derived;
    }
}

mf::KeyboardStateTracker::KeyboardStateTracker()
    : context{xkb_context_new(XKB_CONTEXT_NO_FLAGS), xkb_context_unref}
{
    if (!context)
        fatal_error("KeyboardStateTracker: failed to create XKB context");
}

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

    auto& [scancode_to_keysym, shift_state, xkb_key_state] =
        device_states[input_event->device_id()];

    xkb_key_state.update_keymap(key_event->keymap(), context.get());

    // Keep xkb_key_state in sync with every key event so that its modifier
    // tracking stays accurate for subsequent keysym queries.
    auto const xkb_keycode = to_xkb_scan_code(static_cast<uint32_t>(scancode));
    xkb_key_state.update_key(xkb_keycode, action);

    auto const prev_shift_state = shift_state;
    shift_state = modifiers & (mir_input_event_modifier_shift | mir_input_event_modifier_shift_left |
                               mir_input_event_modifier_shift_right);

    auto processed = false;
    if (action == mir_keyboard_action_down)
    {
        scancode_to_keysym[scancode] = keysym;
        processed = true;
    }
    else if (action == mir_keyboard_action_up)
    {
        // Remove by scancode so that a mismatched key-up keysym (caused by a
        // modifier change while the key was held) does not leave stale entries.
        scancode_to_keysym.erase(scancode);
        processed = true;
    }

    // When the shift state changes, re-derive every pressed keysym from its
    // scancode using the layout-aware XKB state.
    if (prev_shift_state != shift_state)
        xkb_key_state.rederive_keysyms_from_scancodes(scancode_to_keysym);

    return processed;
}

auto mf::KeyboardStateTracker::keysym_is_pressed(MirInputDeviceId device, xkb_keysym_t keysym) const -> bool
{
    if (!device_states.contains(device))
        return false;

    return std::ranges::any_of(
        device_states.at(device).scancode_to_keysym, [keysym](auto const& pair) { return pair.second == keysym; });
}

auto mf::KeyboardStateTracker::scancode_is_pressed(MirInputDeviceId device, int32_t scancode) const -> bool
{
    if (!device_states.contains(device))
        return false;

    return device_states.at(device).scancode_to_keysym.contains(scancode);
}
