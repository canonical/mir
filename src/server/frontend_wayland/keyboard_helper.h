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

#ifndef MIR_FRONTEND_KEYBOARD_HELPER_H
#define MIR_FRONTEND_KEYBOARD_HELPER_H

#include "wayland_wrapper.h"
#include "mir/events/xkb_modifiers.h"

#include <vector>
#include <functional>

struct MirEvent;
struct MirKeyboardEvent;

// from <xkbcommon/xkbcommon.h>
struct xkb_keymap;
struct xkb_state;
struct xkb_context;

namespace mir
{
namespace input
{
class Keymap;
class Seat;
}

namespace frontend
{
class WlSeat;
class KeyboardCallbacks
{
public:
    KeyboardCallbacks() = default;
    virtual ~KeyboardCallbacks() = default;

    virtual void send_repeat_info(int32_t rate, int32_t delay) = 0;
    virtual void send_keymap_xkb_v1(mir::Fd const& fd, size_t length) = 0;
    virtual void send_key(std::shared_ptr<MirKeyboardEvent const> const& event) = 0;
    virtual void send_modifiers(MirXkbModifiers const& modifiers) = 0;

private:
    KeyboardCallbacks(KeyboardCallbacks const&) = delete;
    KeyboardCallbacks& operator=(KeyboardCallbacks const&) = delete;
};

class KeyboardHelper
{
public:
    void handle_event(std::shared_ptr<MirEvent const> const& event);

    /// Returns the scancodes of pressed keys
    auto pressed_key_scancodes() const -> std::vector<uint32_t>;
    /// Updates the modifiers from the seat
    void refresh_modifiers();

private:
    friend class mir::frontend::WlSeat;
    KeyboardHelper(
        KeyboardCallbacks* keybaord_impl,
        std::shared_ptr<mir::input::Keymap> const& initial_keymap,
        std::shared_ptr<input::Seat> const& seat,
        bool enable_key_repeat);

    void handle_keyboard_event(std::shared_ptr<MirKeyboardEvent const> const& event);
    void set_keymap(std::shared_ptr<mir::input::Keymap> const& new_keymap);
    void set_modifiers(MirXkbModifiers const& new_modifiers);

    KeyboardCallbacks* const callbacks;
    std::shared_ptr<input::Seat> const mir_seat;
    MirXkbModifiers modifiers;
    std::shared_ptr<mir::input::Keymap> current_keymap;
    std::unique_ptr<xkb_keymap, void (*)(xkb_keymap *)> compiled_keymap;
    std::unique_ptr<xkb_context, void (*)(xkb_context *)> const context;
};
}
}

#endif // MIR_FRONTEND_KEYBOARD_HELPER_H
