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
#include "mir/log.h"
#include "mir/events/keyboard_event.h"
#include "mir/wayland/client.h"

#include <xkbcommon/xkbcommon.h>
#include <cstring> // memcpy

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;

mf::WlKeyboard::WlKeyboard(wl_resource* new_resource, WlSeat& seat)
    : wayland::Keyboard{new_resource, Version<8>()},
      helper{seat.make_keyboard_helper(this)}
{
}

void mf::WlKeyboard::handle_event(std::shared_ptr<MirEvent const> const& event)
{
    helper->handle_event(event);
}

void mf::WlKeyboard::focus_on(WlSurface* surface)
{
    if (as_nullable_ptr(focused_surface) == surface)
    {
        return;
    }

    if (focused_surface)
    {
        auto const serial = client->next_serial(nullptr);
        send_leave_event(serial, focused_surface.value().raw_resource());
    }

    if (surface)
    {
        // TODO: Send the surface's keymap here

        auto const pressed_keys = helper->pressed_key_scancodes();

        wl_array key_state;
        wl_array_init(&key_state);

        auto* const array_storage = wl_array_add(
            &key_state,
            pressed_keys.size() * sizeof(decltype(pressed_keys)::value_type));

        if (!array_storage)
        {
            wl_resource_post_no_memory(resource);
            BOOST_THROW_EXCEPTION(std::bad_alloc());
        }

        if (!pressed_keys.empty())
        {
            std::memcpy(
                array_storage,
                pressed_keys.data(),
                pressed_keys.size() * sizeof(decltype(pressed_keys)::value_type));
        }

        auto const serial = client->next_serial(nullptr);
        send_enter_event(serial, surface->raw_resource(), &key_state);
        wl_array_release(&key_state);
        mir::log_debug("Surface (id=%u) has been focused and is refreshing its modifiers",
            wl_resource_get_id(surface->raw_resource()));
        helper->refresh_modifiers();
    }

    focused_surface = mw::make_weak(surface);
}

void mf::WlKeyboard::send_repeat_info(int32_t rate, int32_t delay)
{
    send_repeat_info_event_if_supported(rate, delay);
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
    mir::log_debug(
        "Sending modifiers event: depressed=0x%08x, latched=0x%08x, locked=0x%08x, effective_layout=0x%08x",
       modifiers.depressed,
       modifiers.latched,
       modifiers.locked,
       modifiers.effective_layout);

    auto const serial = client->next_serial(nullptr);
    send_modifiers_event(
        serial,
        modifiers.depressed,
        modifiers.latched,
        modifiers.locked,
        modifiers.effective_layout);
}
