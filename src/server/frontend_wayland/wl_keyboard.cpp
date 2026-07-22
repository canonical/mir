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

#include "wl_keyboard.h"

#include "wl_surface.h"
#include "wl_seat.h"
#include "wayland_rs/src/ffi.rs.h"

#include <mir/log.h>
#include <mir/events/keyboard_event.h>

#include <xkbcommon/xkbcommon.h>
#include <cstring> // memcpy

namespace mf = mir::frontend;
namespace mwrs = mir::wayland;

mf::WlKeyboard::WlKeyboard(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::KeyboardMiddleware> instance,
    uint32_t object_id,
    WlSeat& seat)
    : Keyboard{std::move(client), std::move(instance), object_id},
      helper{seat.make_keyboard_helper(this)}
{
}

void mf::WlKeyboard::handle_event(std::shared_ptr<MirEvent const> const& event)
{
    helper->handle_event(event);
}

void mf::WlKeyboard::focus_on(WlSurface* surface)
{
    if (mwrs::as_nullable_ptr(focused_surface) == surface)
    {
        return;
    }

    if (focused_surface)
    {
        auto const serial = client->next_serial(nullptr);
        send_leave_event(serial, as_surface_ptr(&focused_surface.value()));
    }

    if (surface)
    {
        // TODO: Send the surface's keymap here

        auto const pressed_keys = helper->pressed_key_scancodes();
        std::vector<uint8_t> key_state(pressed_keys.size() * sizeof(decltype(pressed_keys)::value_type));

        if (!pressed_keys.empty())
        {
            std::memcpy(
                key_state.data(),
                pressed_keys.data(),
                pressed_keys.size() * sizeof(decltype(pressed_keys)::value_type));
        }

        auto const serial = client->next_serial(nullptr);
        send_enter_event(serial, as_surface_ptr(surface), key_state);
        helper->refresh_modifiers();
    }

    focused_surface = mwrs::make_weak(surface);
}

void mf::WlKeyboard::send_repeat_info(int32_t rate, int32_t delay)
{
    send_repeat_info_event_if_supported(rate, delay);
}

void mf::WlKeyboard::send_keymap_xkb_v1(mir::Fd const& fd, size_t length)
{
    send_keymap_event(KeymapFormat::xkb_v1, static_cast<int32_t>(fd), static_cast<uint32_t>(length));
}

void mf::WlKeyboard::send_key(std::shared_ptr<MirKeyboardEvent const> const& event)
{
    auto const serial = client->next_serial(event);;
    auto const timestamp = mir_input_event_get_wayland_timestamp(event.get());
    int const scancode = event->scan_code();
    auto const state = (event->action() == mir_keyboard_action_down) ? KeyState::pressed : KeyState::released;
    send_key_event(serial, timestamp, scancode, state);
}

void mf::WlKeyboard::send_modifiers(MirXkbModifiers const& modifiers)
{
    auto const serial = client->next_serial(nullptr);
    send_modifiers_event(
        serial,
        modifiers.depressed,
        modifiers.latched,
        modifiers.locked,
        modifiers.effective_layout);
}
