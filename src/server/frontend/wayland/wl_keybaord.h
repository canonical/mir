/*
 * Copyright © 2018 Canonical Ltd.
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

#ifndef WL_KEYBOARD_H
#define WL_KEYBOARD_H

#include "core_generated_interfaces.h"

#include "mir_toolkit/events/event.h"

#include <vector>

// from <xkbcommon/xkbcommon.h>
struct xkb_keymap;
struct xkb_state;
struct xkb_context;

namespace mir
{
class Executor;

namespace input
{
class Keymap;
}
namespace frontend
{
class WlKeyboard : public wayland::Keyboard
{
public:
    WlKeyboard(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        mir::input::Keymap const& initial_keymap,
        std::function<void(WlKeyboard*)> const& on_destroy,
        std::function<std::vector<uint32_t>()> const& acquire_current_keyboard_state,
        std::shared_ptr<mir::Executor> const& executor);

    ~WlKeyboard();

    void handle_event(MirInputEvent const* event, wl_resource* /*target*/);
    void handle_event(MirWindowEvent const* event, wl_resource* target);
    void handle_event(MirKeymapEvent const* event, wl_resource* /*target*/);
    void set_keymap(mir::input::Keymap const& new_keymap);

private:
    void update_modifier_state();

    std::unique_ptr<xkb_keymap, void (*)(xkb_keymap *)> keymap;
    std::unique_ptr<xkb_state, void (*)(xkb_state *)> state;
    std::unique_ptr<xkb_context, void (*)(xkb_context *)> const context;

    std::shared_ptr<mir::Executor> const executor;
    std::function<void(WlKeyboard*)> on_destroy;
    std::function<std::vector<uint32_t>()> const acquire_current_keyboard_state;
    std::shared_ptr<bool> const destroyed;

    uint32_t mods_depressed{0};
    uint32_t mods_latched{0};
    uint32_t mods_locked{0};
    uint32_t group{0};

    void release() override;
};
}
}

#endif // WL_KEYBOARD_H
