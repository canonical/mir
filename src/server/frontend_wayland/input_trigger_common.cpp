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
#include <ranges>
#include <string>
#include <vector>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace sr = std::ranges;

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

bool mf::KeyboardStateTracker::process(MirEvent const& event)
{
    if (event.type() != mir_event_type_input)
        return false;

    auto const& input_event = event.to_input();

    if (input_event->input_type() != mir_input_event_type_key)
        return false;

    auto const* key_event = input_event->to_keyboard();
    auto const keysym = key_event->keysym();
    auto const scancode = key_event->scan_code();
    auto const action = key_event->action();

    if (action == mir_keyboard_action_down)
    {
        // Set all lowercase keys to uppercase
        if (keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R)
        {
            auto const uppercase = sr::views::transform(
                pressed_keysyms,
                [](auto key)
                {
                    if (key >= XKB_KEY_a && key <= XKB_KEY_z)
                        return xkb_keysym_to_upper(key);
                    return key;
                });
            pressed_keysyms = std::unordered_set<xkb_keysym_t>(uppercase.begin(), uppercase.end());
        }

        pressed_keysyms.insert(keysym);
        pressed_scancodes.insert(scancode);
        return true;
    }
    else if (action == mir_keyboard_action_up)
    {
        // Set all uppercase keys to lowercase
        if (keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R)
        {
            auto const lowercase = sr::views::transform(
                pressed_keysyms,
                [](auto key)
                {
                    if (key >= XKB_KEY_A && key <= XKB_KEY_Z)
                        return xkb_keysym_to_lower(key);
                    return key;
                });
            pressed_keysyms = std::unordered_set<xkb_keysym_t>(lowercase.begin(), lowercase.end());
        }

        pressed_keysyms.erase(keysym);
        pressed_scancodes.erase(scancode);
        return true;
    }

    return false;
}

auto mf::KeyboardStateTracker::keysym_is_pressed(uint32_t keysym) const -> bool
{
    return pressed_keysyms.contains(keysym);
}

auto mf::KeyboardStateTracker::scancode_is_pressed(uint32_t scancode) const -> bool
{
    return pressed_scancodes.contains(scancode);
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

auto mf::InputTriggerRegistry::matches_any_trigger(MirEvent const& event) -> bool
{
    if (!keyboard_state.process(event))
        return false;

    bool any_matched{false};

    iterate_and_erase_expired(
        triggers,
        [&](auto& trigger)
        {
            any_matched |= trigger.process(event);
        });

    return any_matched;

}

auto mf::InputTriggerRegistry::keyboard_state_tracker() const -> KeyboardStateTracker const&
{
    return keyboard_state;
}


ActionGroup::ActionGroup(shell::TokenAuthority& token_authority)
    : token_authority{token_authority}
{
}

void ActionGroup::add(wayland::Weak<Action const> action)
{
    actions.push_back(action);
    if (timestamp_and_token)
    {
        auto const& [timestamp, token] = timestamp_and_token.value();
        action.value().begin(timestamp, token);
    }
}

void ActionGroup::send_end(uint32_t wayland_timestamp)
{
    auto const activation_token = static_cast<std::string>(token_authority.issue_token(std::nullopt));
    iterate_and_erase_expired(
        actions, [&](auto const& valid_action) { valid_action.end(wayland_timestamp, activation_token); });

    timestamp_and_token = {};
}

void ActionGroup::send_begin(uint32_t wayland_timestamp)
{
    auto const activation_token = static_cast<std::string>(token_authority.issue_token(std::nullopt));
    timestamp_and_token = {wayland_timestamp, activation_token};

    iterate_and_erase_expired(
        actions, [&](auto const& valid_action) { valid_action.begin(wayland_timestamp, activation_token); });
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

    // Create a pointer to an empty string, not an empty pointer to a string.
    auto token_ptr = std::make_shared<std::string>("");
    auto const ag = std::shared_ptr<ActionGroup>(
        new ActionGroup{*token_authority},
        [this, token_ptr](auto* action_group)
        {
            action_groups.erase(*token_ptr);
            delete action_group;
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

auto ActionGroupManager::get_action_group(std::string const& token) -> std::shared_ptr<ActionGroup> const
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

void Trigger::begin(uint32_t wayland_timestamp)
{
    if (active || !action_group)
        return;

    active = true;
    action_group->send_begin(wayland_timestamp);
}

void Trigger::end(uint32_t wayland_timestamp)
{
    if (!active || !action_group)
        return;

    active = false;
    action_group->send_end(wayland_timestamp);
}

bool Trigger::process(MirEvent const& event)
{
    auto const matched = do_process(event);
    auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());

    if (matched)
        begin(timestamp);
    else
        end(timestamp);

    return matched;
}

bool Trigger::is_same_trigger(KeyboardSymTrigger const*) const
{
    return false;
}

bool Trigger::is_same_trigger(KeyboardCodeTrigger const*) const
{
    return false;
}
