/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_SERVER_FRONTEND_KEYBOARD_STATE_TRACKER_H_
#define MIR_SERVER_FRONTEND_KEYBOARD_STATE_TRACKER_H_

#include <mir/events/event.h>

#include <unordered_set>

namespace mir
{
namespace frontend
{

/// Tracks keyboard state shared among all keyboard event filters
class KeyboardStateTracker
{
public:
    KeyboardStateTracker() = default;

    bool process(MirEvent const& event);

    /// Check if a keysym exists in the pressed set
    auto keysym_is_pressed(uint32_t keysym) const -> bool;

    auto scancode_is_pressed(uint32_t scancode) const -> bool;

private:
    std::unordered_set<uint32_t> pressed_keysyms;
    std::unordered_set<uint32_t> pressed_scancodes;
};

}
}

#endif
