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

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_V1_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_V1_H_

#include "ext-input-trigger-registration-v1_wrapper.h"
#include "input_trigger_registry.h"

#include <cstdint>
#include <string>

namespace mir
{
namespace frontend
{
class KeyboardSymTrigger;
class KeyboardCodeTrigger;
class KeyboardStateTracker;

class InputTriggerV1 : public wayland::InputTriggerV1
{
public:
    using wayland::InputTriggerV1::InputTriggerV1;

    void associate_with_action_group(std::shared_ptr<frontend::ActionGroup> action_group);
    void unassociate_with_action_group(std::shared_ptr<frontend::ActionGroup> action_group);

    bool active() const;
    void begin(std::string const& token, uint32_t wayland_timestamp);
    void end(std::string const& token, uint32_t wayland_timestamp);

    static auto from(struct wl_resource* resource) -> InputTriggerV1*;

    // TODO the usage of a static buffer was a bad idea. Two invocations on the
    // same line clobber each other.
    virtual auto to_c_str() const -> char const* = 0;

    virtual bool is_same_trigger(InputTriggerV1 const* other) const = 0;
    virtual bool is_same_trigger(KeyboardSymTrigger const* sym_trigger) const;
    virtual bool is_same_trigger(KeyboardCodeTrigger const* code_trigger) const;

    virtual bool matches(MirEvent const& event, KeyboardStateTracker const& keyboard_state) const = 0;
private:
    std::shared_ptr<frontend::ActionGroup> action_group;
};

/// Strong type representing modifier flags used internally by Mir.
class InputTriggerModifiers
{
public:
    /// Explicit construction from MirInputEventModifiers
    explicit InputTriggerModifiers(MirInputEventModifiers value);

    /// Get the raw MirInputEventModifiers value (for internal use)
    auto raw_value() const -> MirInputEventModifiers;

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

    static auto modifiers_match(InputTriggerModifiers modifiers, InputTriggerModifiers event_mods) -> bool;

private:
    MirInputEventModifiers value;
};
}
}

#endif
