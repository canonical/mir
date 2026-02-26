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

// TODO rename to InputTriggerActionGroup?
class ActionGroup
{
public:
    void add(wayland::Weak<frontend::InputTriggerActionV1 const> action);

    auto any_trigger_active() const -> bool;

    void end(std::string const& activation_token, uint32_t wayland_timestamp);

    void begin(std::string const& activation_token, uint32_t wayland_timestamp);

private:
    std::vector<wayland::Weak<InputTriggerActionV1 const>> actions;
    std::optional<std::pair<uint32_t, std::string>> timestamp_and_trigger;
};

class InputTriggerRegistry
{
public:
    using Token = std::string;

    InputTriggerRegistry(std::shared_ptr<shell::TokenAuthority> const& token_authority, Executor& wayland_executor);

    auto create_new_action_group() -> std::pair<Token, std::shared_ptr<ActionGroup>>;

    auto register_trigger(frontend::InputTriggerV1* trigger) -> bool;

    bool matches_any_trigger(MirEvent const& event);

    bool was_revoked(Token const& token) const;

    auto get_action_group(Token const& token) -> std::shared_ptr<ActionGroup> const;

private:
    void token_revoked(Token const& token);

    std::unordered_map<Token, std::weak_ptr<ActionGroup>> action_groups;
    std::vector<wayland::Weak<InputTriggerV1>> triggers;
    RecentTokens revoked_tokens;

    std::shared_ptr<shell::TokenAuthority> const token_authority;
    Executor& wayland_executor;

    KeyboardStateTracker keyboard_state;
};
}
}

#endif
