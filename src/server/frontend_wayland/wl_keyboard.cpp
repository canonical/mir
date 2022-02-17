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

#include "wl_surface.h"
#include "wl_seat.h"
#include "mir/log.h"
#include "mir/fatal.h"

#include <xkbcommon/xkbcommon.h>
#include <cstring> // memcpy

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;

mf::WlKeyboard::WlKeyboard(wl_resource* new_resource, WlSeat& seat)
    : wayland::Keyboard{new_resource, Version<6>()},
      seat{seat},
      helper{seat.make_keyboard_helper(this)}
{
    seat.add_focus_listener(client, this);
}

mf::WlKeyboard::~WlKeyboard()
{
    seat.remove_focus_listener(client, this);
}

void mf::WlKeyboard::handle_event(MirInputEvent const* event, WlSurface& surface)
{
    if (!focused_surface.is(surface))
    {
        log_warning(
            "Sending keyboard event to wl_surface@%u even though it was not explicitly given keyboard focus",
            wl_resource_get_id(surface.resource));

        focussed(surface, true);
    }

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
        auto const serial = wl_display_next_serial(wl_client_get_display(client));
        send_leave_event(serial, focused_surface.value().raw_resource());
    }

    focused_surface = mw::make_weak(surface);

    if (surface)
    {
        // TODO: Send the surface's keymap here

        auto const pressed_keys = helper->refresh_internal_state();

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

        auto const serial = wl_display_next_serial(wl_client_get_display(client));
        send_enter_event(serial, surface->raw_resource(), &key_state);
        wl_array_release(&key_state);
    }
}

void mf::WlKeyboard::send_repeat_info(int32_t rate, int32_t delay)
{
    if (version_supports_repeat_info())
    {
        send_repeat_info_event(rate, delay);
    }
}

void mf::WlKeyboard::send_keymap_xkb_v1(mir::Fd const& fd, size_t length)
{
    send_keymap_event(KeymapFormat::xkb_v1, fd, length);
}

void mf::WlKeyboard::send_key(uint32_t timestamp, int scancode, bool down)
{
    auto const serial = wl_display_next_serial(wl_client_get_display(client));
    auto const state = down ? KeyState::pressed : KeyState::released;
    send_key_event(serial, timestamp, scancode, state);
}

void mf::WlKeyboard::send_modifiers(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
{
    auto const serial = wl_display_get_serial(wl_client_get_display(client));
    send_modifiers_event(serial, depressed, latched, locked, group);
}
