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

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_DATA_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_DATA_H_

#include "ext-input-trigger-action-v1_wrapper.h"
#include "input_trigger_registration_v1.h"

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

// TODO move impl to source file
/// Strong type representing modifier flags used internally by Mir.
class InputTriggerModifiers
{
public:
    /// Explicit construction from MirInputEventModifiers
    explicit InputTriggerModifiers(MirInputEventModifiers value) :
        value{value}
    {
    }

    /// Get the raw MirInputEventModifiers value (for internal use)
    auto raw_value() const -> MirInputEventModifiers
    {
        return value;
    }

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

// TODO move impl to source file
class RecentTokens
{
public:
    void add(std::string_view token)
    {
        *current = token;

        if (++current == tokens.end())
        {
            current = tokens.begin();
        }
    }

    auto contains(std::string_view token) const
    {
        return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
    }

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
    /// true, also check for the opposite case of the keysym (if it's an ASCII
    /// letter)
    auto keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool;

    auto scancode_is_pressed(uint32_t scancode) const -> bool;

    auto to_string() const -> std::string;

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

class InputTriggerLifetimeTracker;
class InputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    InputTriggerActionV1(wl_resource* id, std::shared_ptr<InputTriggerLifetimeTracker> const& lifetime_tracker);

private:
    std::shared_ptr<InputTriggerLifetimeTracker> const lifetime_tracker;
};

// Used in `add_new_action` when a client provides a revoked token to call
// `send_unavailable_event`.
class NullInputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    NullInputTriggerActionV1(wl_resource* id) :
        wayland::InputTriggerActionV1{id, Version<1>{}}
    {
    }
};

class InputTriggerData
{
public:
    using Token = std::string;

    InputTriggerData(std::shared_ptr<shell::TokenAuthority> const& token_authority, Executor& wayland_executor);

    auto add_new_action(std::string const& token, struct wl_resource* id) -> bool;

    void add_new_action_control(struct wl_resource* id);

    auto has_trigger(frontend::InputTriggerV1 const* trigger) -> bool;

    bool matches(MirEvent const& event);

    void erase(Token const& token);

private:

    class TokenData
    {
    public:
        bool is_valid(Token const& token) const;

        void add_action(Token const& token, struct wl_resource* id);

        void add_action_control(
            Token const& token,
            std::shared_ptr<InputTriggerLifetimeTracker> const& lifetime_tracker,
            struct wl_resource* id);

        bool has_trigger(frontend::InputTriggerV1 const* trigger) const;

        bool matches(
            MirEvent const& event, KeyboardStateTracker const& keyboard_state, shell::TokenAuthority& token_authority);

        void erase(Token const& token);

    private:

        using TriggerList = std::vector<wayland::Weak<InputTriggerV1 const>>;
        class ActionControl;

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

        // All the data associated with a token that we need to keep track of.
        struct Entry
        {
            // Used by action controls to add or drop triggers, and by
            // TokenData::matches to check for matches when input events
            // arrive.
            TriggerList trigger_list;

            // List of actions associated with the token. Used to send begin
            // and end events when matches are made and broken.
            ActionGroup action_group;

            // If no actions are yet associated with the token, triggers are
            // added and dropped from this pending list, which is copied over to
            // the trigger list once the first action is added. After that,
            // triggers are added and dropped from the active list directly.
            TriggerList pending_triggers;

            // Passed to action controls and actions as a shared_ptr. When the
            // last reference is destroyed, removes the entry from the
            // token_data map.
            std::weak_ptr<InputTriggerLifetimeTracker> lifetime_tracker;
        };

        std::unordered_map<Token, Entry> entries;
    };

    void token_revoked(Token const& token);

    TokenData token_data;
    RecentTokens revoked_tokens;

    std::shared_ptr<shell::TokenAuthority> const token_authority;
    Executor& wayland_executor;

    KeyboardStateTracker keyboard_state;
};
}
}

#endif
