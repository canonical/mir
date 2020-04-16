/*
 * Copyright © 2018-2019 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wl_keyboard.h"

#include "wayland_utils.h"
#include "wl_surface.h"

#include "mir/executor.h"
#include "mir/anonymous_shm_file.h"
#include "mir/input/keymap.h"
#include "mir/log.h"

#include <xkbcommon/xkbcommon.h>
#include <boost/throw_exception.hpp>

#include <cstring> // memcpy

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;

mf::WlKeyboard::WlKeyboard(
    wl_resource* new_resource,
    mir::input::Keymap const& initial_keymap,
    std::function<void(WlKeyboard*)> const& on_destroy,
    std::function<std::vector<uint32_t>()> const& acquire_current_keyboard_state)
    : Keyboard(new_resource, Version<6>()),
      keymap{nullptr, &xkb_keymap_unref},
      state{nullptr, &xkb_state_unref},
      context{xkb_context_new(XKB_CONTEXT_NO_FLAGS), &xkb_context_unref},
      on_destroy{on_destroy},
      acquire_current_keyboard_state{acquire_current_keyboard_state}
{
    // TODO: We should really grab the keymap for the focused surface when
    // we receive focus.

    // TODO: Maintain per-device keymaps, and send the appropriate map before
    // sending an event from a keyboard with a different map.

    /* The wayland::Keyboard constructor has already run, creating the keyboard
     * resource. It is thus safe to send a keymap event to it; the client will receive
     * the keyboard object before this event.
     */
    set_keymap(initial_keymap);

    // I don't know where to get "real" rate and delay args. These are better than nothing.
    if (version_supports_repeat_info())
        send_repeat_info_event(30, 200);
}

mf::WlKeyboard::~WlKeyboard()
{
    on_destroy(this);
}

void mf::WlKeyboard::key(std::chrono::milliseconds const& ms, WlSurface* surface, int scancode, bool down)
{
    if (*focused_surface_destroyed || focused_surface != surface)
    {
        log_warning(
            "Sending key to wl_surface@%u even though it was not explicitly given keyboard focus",
            wl_resource_get_id(surface->resource));
        focussed(surface, true);
    }

    auto const serial = wl_display_next_serial(wl_client_get_display(client));
    /*
     * HACK! Maintain our own XKB state, so we can serialise it for
     * wl_keyboard_send_modifiers
     */
    xkb_key_direction const xkb_state = down ? XKB_KEY_DOWN : XKB_KEY_UP;
    auto const wayland_state = down ? KeyState::pressed : KeyState::released;

    xkb_state_update_key(state.get(), scancode + 8, xkb_state);
    send_key_event(serial, ms.count(), scancode, wayland_state);
    update_modifier_state();
}

void mf::WlKeyboard::focussed(WlSurface* surface, bool should_be_focused)
{
    bool const is_currently_focused = (*focused_surface_destroyed == false && focused_surface == surface);

    if (should_be_focused == is_currently_focused)
        return;

    if (*focused_surface_destroyed == false)
    {
        auto const serial = wl_display_next_serial(wl_client_get_display(client));
        send_leave_event(serial, focused_surface->raw_resource());
    }

    if (should_be_focused)
    {
        // TODO: Send the surface's keymap here

        auto const keyboard_state = acquire_current_keyboard_state();
        update_keyboard_state(keyboard_state);

        wl_array key_state;
        wl_array_init(&key_state);

        auto* const array_storage = wl_array_add(
            &key_state,
            keyboard_state.size() * sizeof(decltype(keyboard_state)::value_type));

        if (!array_storage)
        {
            wl_resource_post_no_memory(resource);
            BOOST_THROW_EXCEPTION(std::bad_alloc());
        }

        if (!keyboard_state.empty())
        {
            std::memcpy(
                array_storage,
                keyboard_state.data(),
                keyboard_state.size() * sizeof(decltype(keyboard_state)::value_type));
        }

        auto const serial = wl_display_next_serial(wl_client_get_display(client));
        send_enter_event(serial, surface->raw_resource(), &key_state);
        wl_array_release(&key_state);

        focused_surface = surface;
        focused_surface_destroyed = surface->destroyed_flag();
    }
    else
    {
        focused_surface = nullptr;
        focused_surface_destroyed = std::make_shared<bool>(true);
    }
}

void mf::WlKeyboard::update_keyboard_state(std::vector<uint32_t> const& keyboard_state)
{
    // Rebuild xkb state
    state = decltype(state)(xkb_state_new(keymap.get()), &xkb_state_unref);
    for (auto scancode : keyboard_state)
    {
        xkb_state_update_key(state.get(), scancode + 8, XKB_KEY_DOWN);
    }

    update_modifier_state();
}

void mf::WlKeyboard::set_keymap(mi::Keymap const& new_keymap)
{
    xkb_rule_names const names = {
        "evdev",
        new_keymap.model.c_str(),
        new_keymap.layout.c_str(),
        new_keymap.variant.c_str(),
        new_keymap.options.c_str()
    };
    keymap = decltype(keymap){
        xkb_keymap_new_from_names(
            context.get(),
            &names,
            XKB_KEYMAP_COMPILE_NO_FLAGS),
        &xkb_keymap_unref};

    // TODO: We might need to copy across the existing depressed keys?
    state = decltype(state)(xkb_state_new(keymap.get()), &xkb_state_unref);

    std::unique_ptr<char, void(*)(void*)> buffer{xkb_keymap_get_as_string(keymap.get(), XKB_KEYMAP_FORMAT_TEXT_V1), free};
    auto length = strlen(buffer.get());

    mir::AnonymousShmFile shm_buffer{length};
    memcpy(shm_buffer.base_ptr(), buffer.get(), length);

    send_keymap_event(KeymapFormat::xkb_v1,
                      Fd{IntOwnedFd{shm_buffer.fd()}},
                      length);
}

void mf::WlKeyboard::update_modifier_state()
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

        send_modifiers_event(wl_display_get_serial(wl_client_get_display(client)),
                             mods_depressed,
                             mods_latched,
                             mods_locked,
                             group);
    }
}

void mf::WlKeyboard::release()
{
    wl_resource_destroy(resource);
}

void mir::frontend::WlKeyboard::resync_keyboard()
{
    update_keyboard_state(acquire_current_keyboard_state());
}
