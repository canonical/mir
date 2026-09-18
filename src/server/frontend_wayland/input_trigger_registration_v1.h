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

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_REGISTRATION_V1_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_REGISTRATION_V1_H_

#include "ext-input-trigger-registration-v1_wrapper.h"
#include "input_trigger_registry.h"

#include <mir_toolkit/events/enums.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mir
{
namespace frontend
{
class KeyboardStateTracker;

/// Strong type representing modifier flags used internally by Mir.
///
/// A trigger's modifier requirement is expressed as two masks: the \c required
/// bits that must all be present in an event, and the \c allowed bits that may
/// additionally be present. Any event modifier outside the allowed set
/// disqualifies a match.
class InputTriggerModifiers
{
public:
    /// Construct directly from Mir modifier masks.
    explicit InputTriggerModifiers(MirInputEventModifiers required, MirInputEventModifiers allowed);

    /// Convert to string for debugging
    auto to_string() const -> std::string;

    /// Explicit conversion from ProtocolModifiers
    static auto from_protocol(uint32_t protocol_mods) -> InputTriggerModifiers;

    /// Explicit conversion from ProtocolModifiers with keysym for shift
    /// adjustment.
    ///
    /// `protocol_mods` is a mask containing the protocol modifier flags (e.g.
    /// Shift, Ctrl, Alt) as defined in ext_input_trigger_registration_v1. A
    /// client could request a trigger with an uppercase letter keysym, but not
    /// provide a shift modifier. "Ctrl + E" for example. The keysym
    /// corresponding to "E" only appears in input events if Shift is pressed.
    /// But since the client did not specify Shift in their original request,
    /// the protocol modifier mask will not contain Shift, and the event
    /// modifier mask will contain Shift, and the trigger won't match. To
    /// account for this, we patch the protocol modifiers at registration time.
    static auto from_protocol(uint32_t protocol_mods, bool shift_adjustment) -> InputTriggerModifiers;

    bool operator==(InputTriggerModifiers const& other) const = default;

    /// \return true if the event modifiers exactly satisfy the trigger: every
    /// required modifier is present and no modifier outside the allowed set is.
    static auto modifiers_match(InputTriggerModifiers modifiers, MirInputEventModifiers event_mods) -> bool;

    /// \return true if the event contains all of the trigger's required
    /// modifiers plus at least one additional modifier group the trigger does
    /// not use. Used to gracefully consume events for a still-held combo when
    /// an unrelated modifier is added, without spuriously matching unrelated
    /// combinations that lack the required modifiers.
    static auto event_modifiers_are_superset(InputTriggerModifiers modifiers, MirInputEventModifiers event_mods) -> bool;

private:
    MirInputEventModifiers const required;
    MirInputEventModifiers const allowed;
};

auto create_input_trigger_registration_manager_v1(
    wl_display* display,
    std::shared_ptr<InputTriggerRegistry::ActionGroupManager> const& action_group_manager,
    std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry,
    std::shared_ptr<KeyboardStateTracker> const& keyboard_state_tracker)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>;
}
}
#endif
