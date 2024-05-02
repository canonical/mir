/*
 * Copyright © Canonical Ltd.
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
#include "mir/log.h"

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

void mf::KeyboardHelper::handle_event(std::shared_ptr<MirEvent const> const& event)
{
    switch (mir_input_event_get_type(mir_event_get_input_event(event.get())))
    {
    case mir_input_event_type_keyboard_resync:
        mir::log_debug("mir_input_event_type_keyboard_resync is causing modifiers to be refreshed");
        refresh_modifiers();
        break;

    case mir_input_event_type_key:
        handle_keyboard_event(dynamic_pointer_cast<MirKeyboardEvent const>(event));
        break;

    default:;
    }
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

void mf::KeyboardHelper::refresh_modifiers()
{
    set_modifiers(mir_seat->xkb_modifiers());
}

void mf::KeyboardHelper::handle_keyboard_event(std::shared_ptr<MirKeyboardEvent const> const& event)
{
    auto const action = mir_keyboard_event_action(event.get());

    set_keymap(event->keymap());
    if (action == mir_keyboard_action_down || action == mir_keyboard_action_up)
    {
        callbacks->send_key(event);
    }
    if (auto const mods = event->xkb_modifiers())
    {
        set_modifiers(mods.value());
    }
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

    std::unique_ptr<char, void(*)(void*)> buffer{xkb_keymap_get_as_string(
        compiled_keymap.get(),
        XKB_KEYMAP_FORMAT_TEXT_V1),
        free};
    // so the null terminator is included
    auto length = strlen(buffer.get()) + 1;

    mir::AnonymousShmFile shm_buffer{length};
    memcpy(shm_buffer.base_ptr(), buffer.get(), length);

    callbacks->send_keymap_xkb_v1(Fd{IntOwnedFd{shm_buffer.fd()}}, length);
}

void mf::KeyboardHelper::set_modifiers(MirXkbModifiers const& new_modifiers)
{
    mir::log_debug(
    "Modifiers are being set from: depressed=0x%08x, latched=0x%08x, locked=0x%08x, effective_layout=0x%08x",
        modifiers.depressed,
        modifiers.latched,
        modifiers.locked,
        modifiers.effective_layout);

    mir::log_debug(
        "Modifiers are being set to: depressed=0x%08x, latched=0x%08x, locked=0x%08x, effective_layout=0x%08x",
        new_modifiers.depressed,
        new_modifiers.latched,
        new_modifiers.locked,
        new_modifiers.effective_layout);

    if (new_modifiers != modifiers)
    {
        modifiers = new_modifiers;
        callbacks->send_modifiers(new_modifiers);
    }
}
