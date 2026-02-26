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

#include "ext-input-trigger-action-v1_wrapper.h"
#include "input_trigger_registration_v1.h"
#include "input_trigger_action_v1.h"

#include <mir/executor.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/event_filter.h>
#include <mir/shell/token_authority.h>
#include <mir/wayland/weak.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace frontend
{

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
    /// Shift, Ctrl, Alt) as defined in ext_input_trigger_registeration_v1. A
    /// client could request a trigger with an uppercase letter keysym, but not
    /// provide a shift modifier. "Ctrl + E" for example. The keysym
    /// corresponding to "E" only appears in input events if Shift is pressed.
    /// But since the client did not specify Shift in their original request,
    /// the protocol modifier mask will not contain Shift, and the event
    /// modifier mask will contain Shift, and the trigger won't match. To
    /// account for this, we patch the protocol modifiers at registration time.
    static auto from_protocol(uint32_t protocol_mods, bool shift_adjustment) -> InputTriggerModifiers;

    auto operator==(InputTriggerModifiers const& other) const -> bool = default;

private:
    MirInputEventModifiers value;
};

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

/// Tracks keyboard state shared among all keyboard event filters
class KeyboardStateTracker
{
public:
    KeyboardStateTracker() = default;

    void on_key_down(uint32_t keysym, uint32_t scancode);
    void on_key_up(uint32_t keysym, uint32_t scancode);

    /// Check if a keysym exists in the pressed set. If `case_insensitive` is
    /// true, also check for the opposite case of the keysym (for a-z, A-Z).
    auto keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool;

    auto scancode_is_pressed(uint32_t scancode) const -> bool;

private:
    std::unordered_set<uint32_t> pressed_keysyms;
    std::unordered_set<uint32_t> pressed_scancodes;
};

class KeyboardSymTrigger;
class KeyboardCodeTrigger;
class InputTriggerV1 : public wayland::InputTriggerV1
{
public:
    using wayland::InputTriggerV1::InputTriggerV1;

    static auto from(struct wl_resource* resource) -> InputTriggerV1*;

    virtual auto to_c_str() const -> char const* = 0;

    virtual bool is_same_trigger(InputTriggerV1 const* other) const = 0;
    virtual bool is_same_trigger(KeyboardSymTrigger const*) const;
    virtual bool is_same_trigger(KeyboardCodeTrigger const*) const;

    virtual bool matches(MirEvent const& event, KeyboardStateTracker const& keyboard_state) const = 0;
};

class KeyboardTrigger : public frontend::InputTriggerV1
{
public:
    KeyboardTrigger(InputTriggerModifiers modifiers, struct wl_resource* id);

    static bool modifiers_match(InputTriggerModifiers protocol_modifiers, InputTriggerModifiers event_mods);

    InputTriggerModifiers const modifiers;
};

class KeyboardSymTrigger : public KeyboardTrigger
{
public:
    KeyboardSymTrigger(InputTriggerModifiers modifiers, uint32_t keysym, struct wl_resource* id);

    auto to_c_str() const -> char const* override;

    auto matches(MirEvent const& ev, KeyboardStateTracker const& keyboard_state) const -> bool override;

    bool is_same_trigger(InputTriggerV1 const* other) const override;
    bool is_same_trigger(KeyboardSymTrigger const* other) const override;

    uint32_t const keysym;
};

class KeyboardCodeTrigger : public KeyboardTrigger
{
public:
    KeyboardCodeTrigger(InputTriggerModifiers modifiers, uint32_t scancode, struct wl_resource* id);

    auto to_c_str() const -> char const* override;

    auto matches(MirEvent const& ev, KeyboardStateTracker const& keyboard_state) const -> bool override;

    bool is_same_trigger(InputTriggerV1 const* other) const override;
    bool is_same_trigger(KeyboardCodeTrigger const* other) const override;

    uint32_t const scancode;
};


// All the data associated with a token that we need to keep track of.
struct InputTriggerTokenData
{
public:
    void add_action(wayland::Weak<frontend::InputTriggerActionV1 const> action);

    void add_trigger(wayland::Weak<frontend::InputTriggerV1 const> trigger);
    void drop_trigger(wayland::Weak<frontend::InputTriggerV1 const> trigger);

    bool has_trigger(frontend::InputTriggerV1 const* trigger) const;

    bool matches(MirEvent const& event, KeyboardStateTracker const& keyboard_state) const;

    bool began() const;
    void begin(std::string const& activation_token, uint32_t wayland_timestamp);
    void end(std::string const& activation_token, uint32_t wayland_timestamp);

private:
    using TriggerList = std::vector<wayland::Weak<InputTriggerV1 const>>;

    struct ActionGroup
    {
    public:
        void add(wayland::Weak<frontend::InputTriggerActionV1 const> action);

        auto began() const -> bool;

        void end(std::string const& activation_token, uint32_t wayland_timestamp);

        void begin(std::string const& activation_token, uint32_t wayland_timestamp);

        bool empty() const;

    private:
        std::vector<wayland::Weak<InputTriggerActionV1 const>> actions;
        bool began_{false};
    };

    // Used by action controls to add or drop triggers, and to check for
    // matches when input events arrive.
    TriggerList trigger_list;

    // List of actions associated with the token. Used to send begin
    // and end events when matches are made and broken.
    ActionGroup action_group;

    // If no actions are yet associated with the token, triggers are
    // added and dropped from this pending list, which is copied over to
    // the trigger list once the first action is added. After that,
    // triggers are added and dropped from the trigger list directly.
    TriggerList pending_triggers;
};

class InputTriggerRegistry
{
public:
    using Token = std::string;

    InputTriggerRegistry(std::shared_ptr<shell::TokenAuthority> const& token_authority, Executor& wayland_executor);

    auto create_new_token_data() -> std::pair<Token, std::shared_ptr<InputTriggerTokenData>>;

    auto has_trigger(frontend::InputTriggerV1 const* trigger) -> bool;

    bool matches_any_trigger(MirEvent const& event);

    bool was_revoked(Token const& token) const;

    auto get_token_data(Token const& token) -> std::shared_ptr<InputTriggerTokenData> const;

private:
    void token_revoked(Token const& token);

    std::unordered_map<Token, std::weak_ptr<InputTriggerTokenData>> token_data;
    RecentTokens revoked_tokens;

    std::shared_ptr<shell::TokenAuthority> const token_authority;
    Executor& wayland_executor;

    KeyboardStateTracker keyboard_state;
};
}
}

#endif
