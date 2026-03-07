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

#ifndef MIR_FRONTEND_WAYLAND_KEYBOARD_STATE_TRACKER_H_
#define MIR_FRONTEND_WAYLAND_KEYBOARD_STATE_TRACKER_H_

#include <mir/events/event.h>

#include <unordered_set>

namespace mir
{
namespace frontend
{
/// Maintains the set of currently-pressed keysyms and scancodes by processing
/// keyboard \c MirEvents via \c process().
///
/// Callers feed every keyboard event into \c process() as it arrives, then
/// query the resulting state with \c keysym_is_pressed() or \c
/// scancode_is_pressed() to test whether a particular key is currently held
/// down. This decouples state accumulation from the individual objects that
/// need to inspect it, so a single tracker instance can be shared across many
/// objects without each one having to maintain its own pressed-key set.
///
/// Shift-key transitions are handled specially: when a Shift key is pressed
/// all lowercase keysyms already in the pressed set are promoted to their
/// uppercase equivalents, and when all Shift keys are released they are
/// demoted back. This keeps the stored keysyms consistent with what the
/// keyboard layer reports as the logical key for subsequent events.
class KeyboardStateTracker
{
public:
    KeyboardStateTracker() = default;

    // Returns true if the passed in event was an up or down keyboard event and
    // was processed, false otherwise.
    bool process(MirEvent const& event);

    auto keysym_is_pressed(MirInputDeviceId device, xkb_keysym_t keysym) const -> bool;

    auto scancode_is_pressed(MirInputDeviceId device, int32_t scancode) const -> bool;

private:
    struct DeviceState
    {
        std::unordered_set<xkb_keysym_t> pressed_keysyms;
        std::unordered_set<int32_t> pressed_scancodes;
        MirInputEventModifiers shift_state{0};
    };

    std::unordered_map<MirInputDeviceId, DeviceState> device_states;
};
}
}


#endif
