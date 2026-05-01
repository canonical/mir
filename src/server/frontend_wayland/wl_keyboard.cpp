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
#include <mir/log.h>
#include <mir/events/keyboard_event.h>

#include <xkbcommon/xkbcommon.h>
#include <cstring> // memcpy

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace mi = mir::input;

mf::WlKeyboard::WlKeyboard(WlSeatGlobal& seat, std::shared_ptr<wayland_rs::Client> const& client)
    : seat{seat},
      client{client}
{
}

auto mf::WlKeyboard::associate(rust::Box<wayland_rs::WlKeyboardExt> instance, uint32_t object_id) -> void
{
    mw::WlKeyboardImpl::associate(std::move(instance), object_id);
    helper = seat.make_keyboard_helper(this);
}

void mf::WlKeyboard::handle_event(std::shared_ptr<MirEvent const> const& event)
{
    helper->handle_event(event);
}

void mf::WlKeyboard::focus_on(WlSurface* surface)
{
    if ((!focused_surface) || &focused_surface.value() == surface)
    {
        return;
    }

    if (focused_surface)
    {
        auto const serial = client->next_serial(nullptr);
        send_leave_event(serial, focused_surface.value().shared_from_this());
    }

    if (surface)
    {
        // TODO: Send the surface's keymap here

        auto const pressed_keys = helper->pressed_key_scancodes();

        size_t total_bytes = pressed_keys.size() * sizeof(uint32_t);
        std::vector<uint8_t> key_state;
        key_state.resize(total_bytes);
        std::memcpy(key_state.data(), pressed_keys.data(), total_bytes);
        auto const serial = client->next_serial(nullptr);
        send_enter_event(serial, surface->shared_from_this(), key_state);
        helper->refresh_modifiers();
    }

    focused_surface = mw::Weak(surface->shared_from_this());
}

void mf::WlKeyboard::send_repeat_info(int32_t rate, int32_t delay)
{
    send_repeat_info_event(rate, delay);
}

void mf::WlKeyboard::send_keymap_xkb_v1(mir::Fd const& fd, size_t length)
{
    send_keymap_event(KeymapFormat::xkb_v1, fd, length);
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
