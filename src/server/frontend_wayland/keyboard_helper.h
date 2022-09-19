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

#ifndef MIR_FRONTEND_KEYBOARD_HELPER_H
#define MIR_FRONTEND_KEYBOARD_HELPER_H

#include "wayland_wrapper.h"

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
class KeyboardCallbacks
{
public:
    KeyboardCallbacks() = default;
    virtual ~KeyboardCallbacks() = default;

    virtual void send_repeat_info(int32_t rate, int32_t delay) = 0;
    virtual void send_keymap_xkb_v1(mir::Fd const& fd, size_t length) = 0;
    virtual void send_key(std::shared_ptr<MirKeyboardEvent const> const& event) = 0;
    virtual void send_modifiers(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) = 0;

private:
    KeyboardCallbacks(KeyboardCallbacks const&) = delete;
    KeyboardCallbacks& operator=(KeyboardCallbacks const&) = delete;
};

class KeyboardHelper
{
public:
    KeyboardHelper(
        KeyboardCallbacks* keybaord_impl,
        std::shared_ptr<mir::input::Keymap> const& initial_keymap,
        std::shared_ptr<input::Seat> const& seat,
        bool enable_key_repeat);

    void handle_event(std::shared_ptr<MirEvent const> const& event);

    /// Returns the scancodes of pressed keys
    auto refresh_internal_state() -> std::vector<uint32_t>;

private:
    auto pressed_key_scancodes() const -> std::vector<uint32_t>;
    void handle_keyboard_event(std::shared_ptr<MirKeyboardEvent const> const& event);
    void set_keymap(std::shared_ptr<mir::input::Keymap> const& new_keymap);
    void update_modifier_state();

    KeyboardCallbacks* const callbacks;
    std::shared_ptr<input::Seat> const mir_seat;
    std::shared_ptr<mir::input::Keymap> current_keymap;
    std::unique_ptr<xkb_keymap, void (*)(xkb_keymap *)> compiled_keymap;
    std::unique_ptr<xkb_state, void (*)(xkb_state *)> state;
    std::unique_ptr<xkb_context, void (*)(xkb_context *)> const context;

    uint32_t mods_depressed{0};
    uint32_t mods_latched{0};
    uint32_t mods_locked{0};
    uint32_t group{0};
};
}
}

#endif // MIR_FRONTEND_KEYBOARD_HELPER_H
