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
#include "input_trigger_registration_v1.h"
#include "mir_toolkit/events/enums.h"

#include <mir/events/keyboard_event.h>
#include <mir/fatal.h>
#include <mir/input/device.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/mir_keyboard_config.h>
#include <mir/input/xkb_mapper.h>

#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include <algorithm>
#include <array>
#include <vector>

namespace mf = mir::frontend;

namespace
{
// xkb scancodes are offset by 8 from evdev scancodes for compatibility with
// the X protocol. This matches the same helper used in xkb_mapper.cpp.
uint32_t constexpr to_xkb_scan_code(uint32_t evdev_scan_code)
{
    return evdev_scan_code + 8;
}

// Mir's modifier flags are based on these physical keys, not XKB modifier names.
constexpr std::array modifier_keys = {
    std::pair{KEY_LEFTSHIFT, mir_input_event_modifier_shift_left},
    std::pair{KEY_RIGHTSHIFT, mir_input_event_modifier_shift_right},
    std::pair{KEY_LEFTCTRL, mir_input_event_modifier_ctrl_left},
    std::pair{KEY_RIGHTCTRL, mir_input_event_modifier_ctrl_right},
    std::pair{KEY_LEFTALT, mir_input_event_modifier_alt_left},
    std::pair{KEY_RIGHTALT, mir_input_event_modifier_alt_right},
    std::pair{KEY_LEFTMETA, mir_input_event_modifier_meta_left},
    std::pair{KEY_RIGHTMETA, mir_input_event_modifier_meta_right},
};
constexpr std::array lock_keys = {KEY_CAPSLOCK, KEY_NUMLOCK, KEY_SCROLLLOCK};
constexpr unsigned shift_keys_mask = 0b11;

auto shortcut_key_matches(
    mf::KeyboardStateTracker::ShortcutKey key, xkb_keycode_t keycode,
    xkb_keymap* keymap, xkb_state* state, bool shift_held, xkb_layout_index_t current_layout) -> bool
{
    if (key.type == mf::KeyboardStateTracker::ShortcutKey::Type::scancode)
        return keycode >= 8 && key.value == keycode - 8;

    if (xkb_state_key_get_one_sym(state, keycode) == key.value)
        return true;

    if (!shift_held)
        return false;

    xkb_keysym_t const* syms{};
    auto const count = xkb_keymap_key_get_syms_by_level(keymap, keycode, current_layout, 0, &syms);
    for (int i = 0; i < count; ++i)
        if (syms[i] == key.value)
            return true;
    return false;
}

auto keymap_shortcuts_overlap(
    xkb_keymap* keymap, mf::KeyboardStateTracker::ShortcutKey first,
    mf::KeyboardStateTracker::ShortcutKey second, unsigned modifier_keys_held, bool shift_held) -> bool
{
    for (unsigned locks = 0; locks < (1u << lock_keys.size()); ++locks)
    {
        auto state = mir::input::XKBStatePtr{xkb_state_new(keymap), xkb_state_unref};
        for (unsigned i = 0; i < modifier_keys.size(); ++i)
            if (modifier_keys_held & (1u << i))
                xkb_state_update_key(state.get(), to_xkb_scan_code(modifier_keys[i].first), XKB_KEY_DOWN);
        for (unsigned i = 0; i < lock_keys.size(); ++i)
        {
            if (locks & (1u << i))
            {
                xkb_state_update_key(state.get(), to_xkb_scan_code(lock_keys[i]), XKB_KEY_DOWN);
                xkb_state_update_key(state.get(), to_xkb_scan_code(lock_keys[i]), XKB_KEY_UP);
            }
        }
        auto const depressed = xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_DEPRESSED);
        auto const latched = xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_LATCHED);
        auto const locked = xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_LOCKED);
        for (xkb_layout_index_t layout = 0; layout < xkb_keymap_num_layouts(keymap); ++layout)
        {
            xkb_state_update_mask(state.get(), depressed, latched, locked, 0, 0, layout);
            for (auto keycode = xkb_keymap_min_keycode(keymap); keycode <= xkb_keymap_max_keycode(keymap); ++keycode)
            {
                // A layout switch does not refresh recorded symbols. Both raw
                // matches must still use the same current group.
                auto const num_layouts = xkb_keymap_num_layouts_for_key(keymap, keycode);
                for (xkb_layout_index_t current = 0; current < num_layouts; ++current)
                {
                    if (shortcut_key_matches(first, keycode, keymap, state.get(), shift_held, current) &&
                        shortcut_key_matches(second, keycode, keymap, state.get(), shift_held, current))
                        return true;
                }
            }
        }
    }
    return false;
}
} // namespace

void mf::KeyboardStateTracker::XkbKeyState::update_keymap(
    std::shared_ptr<mir::input::Keymap> const& new_keymap, xkb_context* context)
{
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

auto mf::KeyboardStateTracker::XkbKeyState::scancode_produces_keysym(
    uint32_t scancode, xkb_keysym_t keysym) const -> bool
{
    if (!compiled_keymap)
        return false;

    auto const xkb_keycode = to_xkb_scan_code(scancode);

    auto const num_layouts = xkb_keymap_num_layouts_for_key(compiled_keymap.get(), xkb_keycode);
    for (auto layout = 0u; layout < num_layouts; ++layout)
    {
        auto const num_levels = xkb_keymap_num_levels_for_key(compiled_keymap.get(), xkb_keycode, layout);
        for (auto level = 0u; level < num_levels; ++level)
        {
            xkb_keysym_t const* syms{};
            auto const num_syms = xkb_keymap_key_get_syms_by_level(
                compiled_keymap.get(), xkb_keycode, layout, level, &syms);
            for (auto i = 0; i < num_syms; ++i)
            {
                if (syms[i] == keysym)
                    return true;
            }
        }
    }
    return false;
}

auto mf::KeyboardStateTracker::XkbKeyState::scancode_has_unshifted_keysym(
    uint32_t scancode, xkb_keysym_t keysym) const -> bool
{
    if (!compiled_keymap)
        return false;

    auto const xkb_keycode = to_xkb_scan_code(scancode);
    auto const active_layout = xkb_state_key_get_layout(state.get(), xkb_keycode);
    if (active_layout == XKB_LAYOUT_INVALID)
        return false;

    xkb_keysym_t const* syms{};
    auto const num_syms = xkb_keymap_key_get_syms_by_level(
        compiled_keymap.get(), xkb_keycode, active_layout, 0, &syms);
    for (auto i = 0; i < num_syms; ++i)
    {
        if (syms[i] == keysym)
            return true;
    }
    return false;
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

mf::KeyboardStateTracker::KeyboardStateTracker(std::shared_ptr<input::InputDeviceHub> const& input_hub)
    : context{xkb_context_new(XKB_CONTEXT_NO_FLAGS), xkb_context_unref},
      input_hub{input_hub}
{
    if (!context)
        MIR_FATAL_ERROR("KeyboardStateTracker: failed to create XKB context");
}

auto mf::KeyboardStateTracker::shortcuts_overlap(
    InputTriggerModifiers first_modifiers, ShortcutKey first,
    InputTriggerModifiers second_modifiers, ShortcutKey second) const -> bool
{
    std::vector<unsigned> common_shift_states;
    for (unsigned keys_held = 0; keys_held < (1u << modifier_keys.size()); ++keys_held)
    {
        MirInputEventModifiers modifiers = 0;
        for (unsigned i = 0; i < modifier_keys.size(); ++i)
            if (keys_held & (1u << i))
                modifiers |= modifier_keys[i].second;
        modifiers = input::expand_modifiers(modifiers) & ~mir_input_event_modifier_none;
        for (unsigned extra = 0; extra < 4; ++extra)
        {
            auto const event_modifiers = modifiers |
                ((extra & 1) ? mir_input_event_modifier_sym : 0) |
                ((extra & 2) ? mir_input_event_modifier_function : 0);
            if (InputTriggerModifiers::modifiers_match(first_modifiers, event_modifiers) &&
                InputTriggerModifiers::modifiers_match(second_modifiers, event_modifiers))
            {
                common_shift_states.push_back(keys_held & shift_keys_mask);
                break;
            }
        }
    }
    if (common_shift_states.empty())
        return false;
    if (first == second)
        return true;
    if (first.type == ShortcutKey::Type::keysym && second.type == ShortcutKey::Type::keysym)
        std::erase(common_shift_states, 0u);
    if (common_shift_states.empty())
        return false;
    if (!input_hub || (first.type == ShortcutKey::Type::scancode && second.type == ShortcutKey::Type::scancode))
        return false;

    std::ranges::sort(common_shift_states);
    common_shift_states.erase(std::unique(common_shift_states.begin(), common_shift_states.end()),
        common_shift_states.end());
    std::vector<std::shared_ptr<input::Keymap>> keymaps;
    input_hub->for_each_input_device([&](input::Device const& device)
        {
            if (auto const config = device.keyboard_configuration())
                keymaps.push_back(config->device_keymap());
        });
    for (auto const& keymap : keymaps)
    {
        auto const compiled = keymap->make_unique_xkb_keymap(context.get());
        for (auto const shift_keys : common_shift_states)
        {
            // Only Shift transitions refresh recorded symbols. Other modifiers
            // can change after key-down without changing the symbol used by a trigger.
            for (unsigned history = 0; history < (1u << modifier_keys.size()); ++history)
            {
                if ((history & shift_keys_mask) != shift_keys)
                    continue;
                if (keymap_shortcuts_overlap(compiled.get(), first, second, history, shift_keys))
                    return true;
            }
        }
    }
    return false;
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

    if (action == mir_keyboard_action_down)
    {
        scancode_to_keysym[scancode] = keysym;
    }
    else if (action == mir_keyboard_action_up)
    {
        // Remove by scancode so that a mismatched key-up keysym (caused by a
        // modifier change while the key was held) does not leave stale entries.
        scancode_to_keysym.erase(scancode);
    }
    else
    {
        // We only care about up and down events.
        return false;
    }

    xkb_key_state.update_keymap(key_event->keymap(), context.get());

    // Keep xkb_key_state in sync with every key event so that its modifier
    // tracking stays accurate for subsequent keysym queries.
    auto const xkb_keycode = to_xkb_scan_code(static_cast<uint32_t>(scancode));
    xkb_key_state.update_key(xkb_keycode, action);

    auto const prev_shift_state = shift_state;
    shift_state = modifiers & (mir_input_event_modifier_shift | mir_input_event_modifier_shift_left |
                               mir_input_event_modifier_shift_right);

    // When the shift state changes, re-derive every pressed keysym from its
    // scancode using the layout-aware XKB state.
    if (prev_shift_state != shift_state)
        xkb_key_state.rederive_keysyms_from_scancodes(scancode_to_keysym);

    return true;
}

auto mf::KeyboardStateTracker::keysym_is_pressed(
    MirInputDeviceId device, xkb_keysym_t keysym, bool include_unshifted) const -> bool
{
    if (!device_states.contains(device))
        return false;

    auto const& device_state = device_states.at(device);
    return std::ranges::any_of(device_state.scancode_to_keysym, [&](auto const& pair)
        {
            return pair.second == keysym ||
                (include_unshifted && device_state.xkb_key_state.scancode_has_unshifted_keysym(pair.first, keysym));
        });
}

auto mf::KeyboardStateTracker::scancode_is_pressed(MirInputDeviceId device, uint32_t scancode) const -> bool
{
    if (!device_states.contains(device))
        return false;

    return device_states.at(device).scancode_to_keysym.contains(scancode);
}

auto mf::KeyboardStateTracker::is_same_key(MirKeyboardEvent const& event, xkb_keysym_t keysym) const -> bool
{
    auto const& device = event.device_id();
    if (!device_states.contains(device))
        return false;

    auto const& [scancode_to_keysym, _, xkb_key_state] = device_states.at(device);
    return xkb_key_state.scancode_produces_keysym(event.scan_code(), keysym);
}
