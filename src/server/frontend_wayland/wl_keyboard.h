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

#ifndef MIR_FRONTEND_WL_KEYBOARD_H
#define MIR_FRONTEND_WL_KEYBOARD_H

#include "keyboard_helper.h"
#include "wayland.h"
#include "weak.h"
#include "wl_seat.h"

namespace mir
{
namespace wayland_rs
{
class Client;
class KeyboardMiddleware;
}
namespace frontend
{
class WlSurface;

class WlKeyboard
    : public wayland_rs::Keyboard,
      private KeyboardCallbacks
{
public:
    WlKeyboard(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::KeyboardMiddleware> instance,
        uint32_t object_id,
        WlSeat& seat);

    void handle_event(std::shared_ptr<MirEvent const> const& event);
    void focus_on(WlSurface* surface);

private:
    std::shared_ptr<KeyboardHelper> const helper;
    wayland_rs::Weak<WlSurface> focused_surface;

    /// KeyboardCallbacks overrides
    /// @{
    void send_repeat_info(int32_t rate, int32_t delay) override;
    void send_keymap_xkb_v1(mir::Fd const& fd, size_t length) override;
    void send_key(std::shared_ptr<MirKeyboardEvent const> const& event) override;
    void send_modifiers(MirXkbModifiers const& modifiers) override;
    /// @}
};
}
}

#endif // MIR_FRONTEND_WL_KEYBOARD_H
