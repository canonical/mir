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

#include "input_trigger_common.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/input/xkb_mapper.h>
#include <mir/log.h>
#include <mir_toolkit/events/enums.h>

#include <algorithm>
#include <string>
#include <vector>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace sr = std::ranges;

using ActivationToken = mf::InputTriggerRegistry::ActivationToken;
using Action = mf::InputTriggerRegistry::Action;
using ActionGroup = mf::InputTriggerRegistry::ActionGroup;
using ActionGroupManager = mf::InputTriggerRegistry::ActionGroupManager;
using Trigger = mf::InputTriggerRegistry::Trigger;


void mf::RecentTokens::add(std::string_view token)
{
    *current = token;

    if (++current == tokens.end())
    {
        current = tokens.begin();
    }
}

auto mf::RecentTokens::contains(std::string_view token) const -> bool
{
    return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

template <typename T>
void iterate_and_erase_expired(
    std::vector<mir::wayland::Weak<T>>& vec, auto&& callback)
{
    std::erase_if(
        vec,
        [&](auto& action)
        {
            if (!action)
                return true; // Erase

            callback(action.value());
            return false; // Don't erase
        });
}

mf::InputTriggerRegistry::InputTriggerRegistry() = default;

bool mf::InputTriggerRegistry::register_trigger(Trigger* trigger)
{
    // Housekeeping
    std::erase_if(triggers, [](auto const& weak_trigger) { return !weak_trigger; });

    auto const already_registered = sr::any_of(
        triggers, [trigger](auto const& other_trigger) { return trigger->is_same_trigger(&other_trigger.value()); });

    if (already_registered)
        return false;

    triggers.push_back(wayland::make_weak(trigger));
    return true;
}

auto mf::InputTriggerRegistry::any_trigger_handled(MirEvent const& event) -> bool
{
    bool any_handled{false};

    iterate_and_erase_expired(
        triggers,
        [&](auto& trigger)
        {
            any_handled |= trigger.handle(event);
        });

    return any_handled;

}

ActivationToken::ActivationToken(MirInputEvent const& event, shell::TokenAuthority& token_authority) :
    wayland_timestamp{mir_input_event_get_wayland_timestamp(&event)},
    token{token_authority.issue_token(std::nullopt)}
{
}

auto ActivationToken::timestamp_ms() const -> uint32_t
{
    return wayland_timestamp;
}

auto ActivationToken::token_string() const& -> char const*
{
    return token.c_str();
}

ActionGroup::ActionGroup(
    std::shared_ptr<shell::TokenAuthority> const& token_authority, std::function<void()>&& on_destroy) :
    on_destroy{std::move(on_destroy)},
    token_authority{token_authority}
{
}

ActionGroup::~ActionGroup()
{
    on_destroy();
}

void ActionGroup::add(wayland::Weak<Action const> action)
{
    actions.push_back(action);
    if (activation_token)
    {
        action.value().begin(*activation_token);
    }
}

void ActionGroup::send_end(MirEvent const& event)
{
    iterate_and_erase_expired(
        actions,
        [activation_token = ActivationToken{*event.to_input(), *token_authority}](auto const& valid_action)
        { valid_action.end(activation_token); });

    activation_token.reset();
}

void ActionGroup::send_begin(MirEvent const& event)
{
    activation_token = ActivationToken{*event.to_input(), *token_authority};

    iterate_and_erase_expired(
        actions, [&](auto const& valid_action) { valid_action.begin(*activation_token); });
}

void ActionGroup::cancel()
{
    cancelled = true;

    for (auto& action : actions)
    {
        if (action)
            action.value().unavailable();
    }

    actions.clear();
}

bool mir::frontend::InputTriggerRegistry::ActionGroup::was_cancelled() const
{
    return cancelled;
}

ActionGroupManager::ActionGroupManager(
    std::shared_ptr<msh::TokenAuthority> const& token_authority, Executor& wayland_executor) :
    token_authority{token_authority},
    wayland_executor{wayland_executor}
{
}

auto ActionGroupManager::create_new_action_group() -> std::pair<std::string, std::shared_ptr<ActionGroup>>
{
    // Ugly, but we somehow need to tie the action group's lifetime to the
    // token, while using the token to remove the entry in action_groups.

    auto token_ptr = std::make_shared<std::string>();
    auto const ag = std::make_shared<ActionGroup>(
        token_authority,
        [this, token_ptr]()
        {
            action_groups.erase(*token_ptr);
        });

    auto const token = token_authority->issue_token(
        [this, ag](auto const& token)
        { wayland_executor.spawn([this, token, ag] { revoked_tokens.add(static_cast<std::string>(token)); }); });

    *token_ptr = static_cast<std::string>(token);

    action_groups.emplace(token, ag);

    return {static_cast<std::string>(token), ag};
}

bool ActionGroupManager::was_revoked(std::string const& token) const
{
    return revoked_tokens.contains(token);
}

auto ActionGroupManager::get_action_group(std::string const& token) -> std::shared_ptr<ActionGroup>
{
    if (auto const iter = action_groups.find(token); iter != action_groups.end())
        return iter->second.lock();
    return nullptr;
}

void Trigger::associate_with_action_group(std::shared_ptr<ActionGroup> action_group)
{
    this->action_group = action_group;
}

void Trigger::unassociate_with_action_group(std::shared_ptr<ActionGroup> action_group)
{
    if (this->action_group == action_group)
        this->action_group.reset();
}

void Trigger::begin(MirEvent const& event)
{
    if (!action_group)
        return;

    action_group->send_begin(event);
}

void Trigger::end(MirEvent const& event)
{
    if (!action_group)
        return;

    action_group->send_end(event);
}

bool Trigger::handle(MirEvent const& event)
{
    switch (check_transition(event)) {
        case Transition::activated:
            begin(event);
            return true;
        case Transition::deactivated:
            end(event);
            return true;
        case Transition::pass:
           return false;
        case Transition::consume:
            return true;
    }
}

bool Trigger::is_same_trigger(KeyboardSymTrigger const*) const
{
    return false;
}

bool Trigger::is_same_trigger(KeyboardCodeTrigger const*) const
{
    return false;
}
