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

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_COMMON_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_COMMON_H_


#include <mir/events/event.h>
#include <mir/executor.h>
#include <mir/shell/token_authority.h>
#include <mir/wayland/lifetime_tracker.h>
#include <mir/wayland/weak.h>

#include <string>
#include <unordered_map>

namespace mir
{
namespace frontend
{
class KeyboardSymTrigger;
class KeyboardCodeTrigger;
class KeyboardStateTracker;

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

// Version-agnostic interface
class InputTriggerAction : public virtual wayland::LifetimeTracker
{
public:
    virtual ~InputTriggerAction() = default;

    virtual void end(uint32_t wayland_timestamp, std::string const& activation_token) const = 0;
    virtual void begin(uint32_t wayland_timestamp, std::string const& activation_token) const = 0;
};

class ActionGroup
{
public:
    ActionGroup(shell::TokenAuthority& token_authority);

    void add(wayland::Weak<InputTriggerAction const> action);
    void end(uint32_t wayland_timestamp);
    void begin(uint32_t wayland_timestamp);

    auto any_trigger_active() const -> bool;
private:
    shell::TokenAuthority& token_authority;
    std::vector<wayland::Weak<InputTriggerAction const>> actions;
    std::optional<std::pair<uint32_t, std::string>> timestamp_and_token;
};

class InputTrigger: public virtual wayland::LifetimeTracker
{
public:
    virtual ~InputTrigger() = default;

    void associate_with_action_group(std::shared_ptr<frontend::ActionGroup> action_group);
    void unassociate_with_action_group(std::shared_ptr<frontend::ActionGroup> action_group);

    // \return true if event was handled, false otherwise.
    bool process(MirEvent const& event);

    virtual auto to_string() const -> std::string = 0;

    virtual bool is_same_trigger(InputTrigger const* other) const = 0;
    virtual bool is_same_trigger(KeyboardSymTrigger const* sym_trigger) const;
    virtual bool is_same_trigger(KeyboardCodeTrigger const* code_trigger) const;

private:
    void begin(uint32_t wayland_timestamp);
    void end(uint32_t wayland_timestamp);

    virtual bool do_process(MirEvent const& event) = 0;

    std::shared_ptr<frontend::ActionGroup> action_group;
    bool active{false};
};

/// Tracks keyboard state shared among all keyboard event filters
class KeyboardStateTracker
{
public:
    KeyboardStateTracker() = default;

    bool process(MirEvent const& event);

    /// Check if a keysym exists in the pressed set. If `case_insensitive` is
    /// true, also check for the opposite case of the keysym (for a-z, A-Z).
    auto keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool;

    auto scancode_is_pressed(uint32_t scancode) const -> bool;

private:
    std::unordered_set<uint32_t> pressed_keysyms;
    std::unordered_set<uint32_t> pressed_scancodes;
};

class InputTriggerRegistry
{
public:
    InputTriggerRegistry();

    bool register_trigger(InputTrigger* trigger);
    bool matches_any_trigger(MirEvent const& event);

    auto keyboard_state_tracker() const -> KeyboardStateTracker const&;

private:
    std::vector<wayland::Weak<InputTrigger>> triggers;
    KeyboardStateTracker keyboard_state;
};

class ActionGroupManager
{
public:
    ActionGroupManager(std::shared_ptr<shell::TokenAuthority> const& token_authority, Executor& wayland_executor);

    auto create_new_action_group() -> std::pair<std::string, std::shared_ptr<ActionGroup>>;

    bool was_revoked(std::string const& token) const;

    auto get_action_group(std::string const& token) -> std::shared_ptr<ActionGroup> const;

private:
    std::unordered_map<std::string, std::weak_ptr<ActionGroup>> action_groups;
    RecentTokens revoked_tokens;

    std::shared_ptr<shell::TokenAuthority> const token_authority;
    Executor& wayland_executor;
};
}
}

#endif
