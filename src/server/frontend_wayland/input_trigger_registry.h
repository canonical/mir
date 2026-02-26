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
#include <mir/wayland/weak.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace frontend
{
class InputTriggerActionV1;
class InputTriggerV1;

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


// All the data associated with a token that we need to keep track of.
class InputTriggerTokenData
{
public:
    void add_action(wayland::Weak<frontend::InputTriggerActionV1 const> action);

    void add_trigger(wayland::Weak<frontend::InputTriggerV1 const> trigger);
    void drop_trigger(wayland::Weak<frontend::InputTriggerV1 const> trigger);

    bool has_trigger(frontend::InputTriggerV1 const* trigger) const;

    bool matches(MirEvent const& event, KeyboardStateTracker const& keyboard_state) const;

    bool any_trigger_active() const;
    void begin(std::string const& activation_token, uint32_t wayland_timestamp);
    void end(std::string const& activation_token, uint32_t wayland_timestamp);

private:
    using TriggerList = std::vector<wayland::Weak<InputTriggerV1 const>>;

    class ActionGroup
    {
    public:
        void add(wayland::Weak<frontend::InputTriggerActionV1 const> action);

        auto any_trigger_active() const -> bool;

        void end(std::string const& activation_token, uint32_t wayland_timestamp);

        void begin(std::string const& activation_token, uint32_t wayland_timestamp);

        bool empty() const;

    private:
        std::vector<wayland::Weak<InputTriggerActionV1 const>> actions;
        std::optional<std::pair<uint32_t, std::string>> timestamp_and_trigger;
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
