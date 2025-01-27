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

#ifndef MIR_FRONTEND_INPUT_METHOD_GRAB_KEYBOARD_V2_H
#define MIR_FRONTEND_INPUT_METHOD_GRAB_KEYBOARD_V2_H

#include "mir/wayland/weak.h"

#include "input-method-unstable-v2_wrapper.h"
#include "keyboard_helper.h"

namespace mir
{
class Executor;
namespace input
{
class CompositeEventFilter;
}
namespace wayland
{
class Client;
}
namespace frontend
{
class WlSeat;
class WlClient;

/// A keyboard that sends all key events to it's client without ever entering a surface
class InputMethodGrabKeyboardV2
    : public wayland::InputMethodKeyboardGrabV2,
      public KeyboardCallbacks
{
public:
    InputMethodGrabKeyboardV2(
        wl_resource* resource,
        WlSeat& seat,
        std::shared_ptr<Executor> const& wayland_executor,
        input::CompositeEventFilter& event_filter);

private:
    class Handler;

    std::shared_ptr<Handler> const handler;
    std::shared_ptr<KeyboardHelper> const helper;
    wayland::Weak<wayland::Client> wl_client;

    /// KeyboardImpl overrides
    /// @{
    void send_repeat_info(int32_t rate, int32_t delay) override;
    void send_keymap_xkb_v1(mir::Fd const& fd, size_t length) override;
    void send_key(std::shared_ptr<MirKeyboardEvent const> const& event) override;
    void send_modifiers(MirXkbModifiers const& modifiers) override;
    /// @}
};
}
}

#endif // MIR_FRONTEND_INPUT_METHOD_GRAB_KEYBOARD_V2_H
