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

#include "mir_toolkit/events/enums.h"
#include <mir/events/event.h>
#include <mir/input/keymap.h>

#include <memory>
#include <unordered_map>

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
/// Shift-key transitions are handled specially: when the shift state changes,
/// all currently-pressed keysyms are re-derived from their scancodes using the
/// layout-aware XKB state. This keeps the stored keysyms consistent with what
/// the keyboard layer reports as the logical key for subsequent events.
///
/// Keysyms are stored per scancode so that a key-up event always removes the
/// keysym that was recorded at key-down, regardless of any modifier changes
/// that occurred while the key was held (e.g. pressing Shift while holding
/// a digit key).
class KeyboardStateTracker
{
public:
    KeyboardStateTracker();

    // Returns true if the passed in event was an up or down keyboard event and
    // was processed, false otherwise.
    bool process(MirEvent const& event);

    auto keysym_is_pressed(MirInputDeviceId device, xkb_keysym_t keysym) const -> bool;

    auto scancode_is_pressed(MirInputDeviceId device, int32_t scancode) const -> bool;

private:
    /// Owns the per-device XKB keymap and state used to resolve keysyms from
    /// scancodes in a layout- and modifier-aware way.
    struct XkbKeyState
    {
        /// Update the compiled keymap and XKB state when a new keymap arrives.
        /// \param new_keymap  The keymap carried on the incoming event.
        /// \param context     The shared XKB context owned by the tracker.
        void update_keymap(std::shared_ptr<mir::input::Keymap> const& new_keymap, xkb_context* context);

        /// Feed a key press or release into the XKB state so that modifier
        /// tracking stays accurate for subsequent keysym queries.
        /// \param xkb_keycode  The XKB keycode for the key.
        /// \param action       The keyboard action.
        void update_key(uint32_t xkb_keycode, MirKeyboardAction action);

        /// Re-derive every keysym in \a scancode_to_keysym from its scancode
        /// using the current modifier state of the XKB state machine.
        void rederive_keysyms_from_scancodes(
            std::unordered_map<uint32_t, xkb_keysym_t>& scancode_to_keysym) const;

    private:
        /// The keymap currently in use for this device, used to detect keymap
        /// changes and rebuild compiled_keymap/state when they occur.
        std::shared_ptr<mir::input::Keymap> current_keymap;

        /// The compiled keymap and live XKB state, kept in sync with every
        /// key event so that modifier state is accurate for keysym queries.
        std::unique_ptr<xkb_keymap, void(*)(xkb_keymap*)> compiled_keymap{nullptr, xkb_keymap_unref};
        std::unique_ptr<xkb_state, void(*)(xkb_state*)> state{nullptr, xkb_state_unref};
    };

    struct DeviceState
    {
        /// Maps each currently-pressed scancode to the keysym that was recorded
        /// when it was pressed (updated on shift-state transitions).
        std::unordered_map<uint32_t, xkb_keysym_t> scancode_to_keysym;
        MirInputEventModifiers shift_state{0};
        XkbKeyState xkb_key_state;
    };

    /// Shared XKB context — one per tracker, reused across all devices.
    std::unique_ptr<xkb_context, void(*)(xkb_context*)> context;

    std::unordered_map<MirInputDeviceId, DeviceState> device_states;
};
}
}


#endif
