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

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_REGISTRY_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_REGISTRY_H_


#include <mir/events/event.h>
#include <mir/executor.h>
#include <mir/shell/token_authority.h>
#include <mir/wayland/lifetime_tracker.h>
#include <mir/wayland/weak.h>
#include <mir_toolkit/events/enums.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mir
{
namespace frontend
{
class KeyboardSymTrigger;
class KeyboardCodeTrigger;

/// A fixed-capacity circular buffer of recently seen tokens.
///
/// Tracks the last \c capacity tokens added, allowing efficient membership
/// checks. Older tokens are evicted as new ones are inserted once the buffer
/// is full.
class RecentTokens
{
public:
    void add(std::string_view token);

    auto contains(std::string_view token) const -> bool;

    static constexpr auto capacity = 32;
private:
    std::array<std::string, capacity> tokens;
    decltype(tokens)::iterator current{tokens.begin()};
};

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

class InputTriggerRegistry
{
public:
    class ActivationToken;
    class Action;
    class ActionGroup;
    class ActionGroupManager;
    class Trigger;

    InputTriggerRegistry();

    bool register_trigger(Trigger* trigger);
    bool any_trigger_handled(MirEvent const& event);

private:
    std::vector<wayland::Weak<Trigger>> triggers;
};

class InputTriggerRegistry::ActivationToken
{
public:
    ActivationToken(MirInputEvent const& event, shell::TokenAuthority& token_authority);

    auto timestamp_ms() const -> uint32_t;
    auto token_string() const& -> char const*;

private:
    uint32_t wayland_timestamp;
    std::string token;
};

// Version-agnostic interface
class InputTriggerRegistry::Action : public virtual wayland::LifetimeTracker
{
public:
    Action() = default;
    virtual ~Action() = default;

    Action(Action const&) = delete;
    auto operator=(Action const&) -> Action& = delete;
    Action(Action&&) = delete;
    auto operator=(Action&&) -> Action& = delete;

    virtual void end(ActivationToken const& activation_token) const = 0;
    virtual void begin(ActivationToken const& activation_token) const = 0;
    virtual void unavailable() const = 0;
};

class InputTriggerRegistry::ActionGroup
{
public:
    ActionGroup(std::shared_ptr<shell::TokenAuthority> const& token_authority, std::function<void()>&& on_destroy);
    ~ActionGroup();

    ActionGroup(ActionGroup const&) = delete;
    auto operator=(ActionGroup const&) -> ActionGroup& = delete;
    ActionGroup(ActionGroup&&) = delete;
    auto operator=(ActionGroup&&) -> ActionGroup& = delete;

    void add(wayland::Weak<Action const> action);
    void send_end(MirEvent const& event);
    void send_begin(MirEvent const& event);
    void cancel();
    bool was_cancelled() const;

private:
    std::function<void()> const on_destroy{};
    std::shared_ptr<shell::TokenAuthority> token_authority;
    std::vector<wayland::Weak<Action const>> actions;
    std::optional<ActivationToken> activation_token;
    bool cancelled{false};
};

class InputTriggerRegistry::ActionGroupManager
{
public:
    ActionGroupManager(std::shared_ptr<shell::TokenAuthority> const& token_authority, Executor& wayland_executor);

    auto create_new_action_group() -> std::pair<std::string, std::shared_ptr<ActionGroup>>;

    bool was_revoked(std::string const& token) const;

    auto get_action_group(std::string const& token) -> std::shared_ptr<ActionGroup>;

private:
    std::unordered_map<std::string, std::weak_ptr<ActionGroup>> action_groups;
    RecentTokens revoked_tokens;

    std::shared_ptr<shell::TokenAuthority> const token_authority;
    Executor& wayland_executor;
};

class InputTriggerRegistry::Trigger: public virtual wayland::LifetimeTracker
{
public:
    enum class EventOutcome
    {
        activated,   // trigger just transitioned from inactive → active (rising edge)
        deactivated, // trigger just transitioned from active → inactive (falling edge)
        pass,        // event is not relevant to this trigger; let it through
        consume,     // event is related to this trigger but doesn't change state; eat it silently
    };

    Trigger() = default;
    virtual ~Trigger() = default;

    Trigger(Trigger const&) = delete;
    auto operator=(Trigger const&) -> Trigger& = delete;
    Trigger(Trigger&&) = delete;
    auto operator=(Trigger&&) -> Trigger& = delete;

    void associate_with_action_group(std::shared_ptr<ActionGroup> action_group);
    void unassociate_with_action_group(std::shared_ptr<ActionGroup> action_group);

    // \return true if the event was handled and should be consumed.
    auto handle(MirEvent const& event) -> bool;

    virtual bool is_same_trigger(Trigger const* other) const = 0;
    virtual bool is_same_trigger(KeyboardSymTrigger const* sym_trigger) const;
    virtual bool is_same_trigger(KeyboardCodeTrigger const* code_trigger) const;

private:
    void begin(MirEvent const& event);
    void end(MirEvent const& event);

    virtual bool is_active() const = 0;

    virtual auto check_event(MirEvent const& event) -> EventOutcome = 0;

    std::shared_ptr<ActionGroup> action_group;
};

}
}

#endif
