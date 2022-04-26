/*
 * Copyright © 2018-2021 Canonical Ltd.
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
 */

#include "keyboard_helper.h"

#include "mir/anonymous_shm_file.h"
#include "mir/input/keymap.h"
#include "mir/events/keyboard_event.h"
#include "mir/input/seat.h"
#include "mir/fatal.h"

#include <xkbcommon/xkbcommon.h>
#include <cstring> // memcpy
#include <unordered_set>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;

mf::KeyboardHelper::KeyboardHelper(
    KeyboardCallbacks* callbacks,
    std::shared_ptr<mi::Keymap> const& initial_keymap,
    std::shared_ptr<input::Seat> const& seat,
    bool enable_key_repeat)
    : callbacks{callbacks},
      mir_seat{seat},
      current_keymap{nullptr}, // will be set later in the constructor by set_keymap()
      compiled_keymap{nullptr, &xkb_keymap_unref},
      state{nullptr, &xkb_state_unref},
      context{xkb_context_new(XKB_CONTEXT_NO_FLAGS), &xkb_context_unref}
{
    if (!context)
    {
        fatal_error("Failed to create XKB context");
    }

    /* The wayland::Keyboard constructor has already run, creating the keyboard
     * resource. It is thus safe to send a keymap event to it; the client will receive
     * the keyboard object before this event.
     */
    set_keymap(initial_keymap);

    // 25 rate and 600 delay are the default in Weston and Sway
    // At some point we will want to make this configurable
    callbacks->send_repeat_info(enable_key_repeat ? 25 : 0, 600);
}

void mf::KeyboardHelper::handle_event(MirInputEvent const* event)
{
    switch (mir_input_event_get_type(event))
    {
    case mir_input_event_type_keyboard_resync:
        refresh_internal_state();
        break;

    case mir_input_event_type_key:
        handle_keyboard_event(mir_input_event_get_keyboard_event(event));
        break;

    default:;
    }
}

auto mf::KeyboardHelper::refresh_internal_state() -> std::vector<uint32_t>
{
    auto const pressed_keys{pressed_key_scancodes()};
    // Rebuild xkb state
    state = decltype(state)(xkb_state_new(compiled_keymap.get()), &xkb_state_unref);
    for (auto scancode : pressed_keys)
    {
        xkb_state_update_key(state.get(), scancode + 8, XKB_KEY_DOWN);
    }

    update_modifier_state();

    return pressed_keys;
}

auto mf::KeyboardHelper::pressed_key_scancodes() const -> std::vector<uint32_t>
{
    std::unordered_set<uint32_t> pressed_keys;
    auto const ev = mir_seat->create_device_state();
    auto const state_event = mir_event_get_input_device_state_event(ev.get());
    for (
        auto dev = 0u;
        dev < mir_input_device_state_event_device_count(state_event);
        ++dev)
    {
        for (
            auto idx = 0u;
            idx < mir_input_device_state_event_device_pressed_keys_count(state_event, dev);
            ++idx)
        {
            pressed_keys.insert(
                mir_input_device_state_event_device_pressed_keys_for_index(
                    state_event,
                    dev,
                    idx));
        }
    }
    return std::vector<uint32_t>{pressed_keys.begin(), pressed_keys.end()};
}

void mf::KeyboardHelper::handle_keyboard_event(MirKeyboardEvent const* event)
{
    bool down;
    switch (mir_keyboard_event_action(event))
    {
    case mir_keyboard_action_down:
        down = true;
        break;

    case mir_keyboard_action_up:
        down = false;
        break;

    default:
        return;
    }

    auto const timestamp = mir_input_event_get_wayland_timestamp(mir_keyboard_event_input_event(event));
    int const scancode = mir_keyboard_event_scan_code(event);

    set_keymap(event->keymap());
    callbacks->send_key(timestamp, scancode, down);
    /*
    * HACK! Maintain our own XKB state, so we can serialise it for
    * wl_keyboard_send_modifiers
    */
    xkb_key_direction const xkb_state = down ? XKB_KEY_DOWN : XKB_KEY_UP;
    xkb_state_update_key(state.get(), scancode + 8, xkb_state);
    update_modifier_state();
}

void mf::KeyboardHelper::set_keymap(std::shared_ptr<mi::Keymap> const& new_keymap)
{
    if (!new_keymap || new_keymap == current_keymap)
    {
        return;
    }

    if (current_keymap && current_keymap->matches(*new_keymap))
    {
        current_keymap = new_keymap;
        return;
    }

    current_keymap = new_keymap;
    compiled_keymap = new_keymap->make_unique_xkb_keymap(context.get());

    // TODO: We might need to copy across the existing depressed keys?
    state = decltype(state)(xkb_state_new(compiled_keymap.get()), &xkb_state_unref);

    std::unique_ptr<char, void(*)(void*)> buffer{xkb_keymap_get_as_string(
        compiled_keymap.get(),
        XKB_KEYMAP_FORMAT_TEXT_V1),
        free};
    auto length = strlen(buffer.get());

    mir::AnonymousShmFile shm_buffer{length};
    memcpy(shm_buffer.base_ptr(), buffer.get(), length);

    callbacks->send_keymap_xkb_v1(Fd{IntOwnedFd{shm_buffer.fd()}}, length);
}

void mf::KeyboardHelper::update_modifier_state()
{
    // TODO?
    // assert_on_wayland_event_loop()

    auto new_depressed_mods = xkb_state_serialize_mods(
        state.get(),
        XKB_STATE_MODS_DEPRESSED);
    auto new_latched_mods = xkb_state_serialize_mods(
        state.get(),
        XKB_STATE_MODS_LATCHED);
    auto new_locked_mods = xkb_state_serialize_mods(
        state.get(),
        XKB_STATE_MODS_LOCKED);
    auto new_group = xkb_state_serialize_layout(
        state.get(),
        XKB_STATE_LAYOUT_EFFECTIVE);

    if ((new_depressed_mods != mods_depressed) ||
        (new_latched_mods != mods_latched) ||
        (new_locked_mods != mods_locked) ||
        (new_group != group))
    {
        mods_depressed = new_depressed_mods;
        mods_latched = new_latched_mods;
        mods_locked = new_locked_mods;
        group = new_group;

        callbacks->send_modifiers(mods_depressed, mods_latched, mods_locked, group);
    }
}
