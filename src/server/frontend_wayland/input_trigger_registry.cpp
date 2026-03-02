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

#include "input_trigger_registry.h"
#include "input_trigger_v1.h"

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
            auto const lowercase_keysyms =
                sr::views::filter(pressed_keysyms, [](auto key) { return key >= XKB_KEY_a && key <= XKB_KEY_z; }) |
                sr::to<std::unordered_set<xkb_keysym_t>>();

            // Doing `pressed_keysyms.erase(begin(), end())` crashes for some
            // reason, so we have to do it one by one. Not sure if it's a me
            // issue or a stdlib bug.
            for (auto const key : lowercase_keysyms)
                pressed_keysyms.erase(key);

            auto const uppercase_keysyms = sr::views::transform(lowercase_keysyms, [](auto key) { return xkb_keysym_to_upper(key); });
            pressed_keysyms.insert(uppercase_keysyms.begin(), uppercase_keysyms.end());
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
            auto const uppercase_keysyms =
                sr::views::filter(pressed_keysyms, [](auto key) { return key >= XKB_KEY_A && key <= XKB_KEY_Z; }) |
                sr::to<std::unordered_set<xkb_keysym_t>>();

            for (auto const key : uppercase_keysyms)
                pressed_keysyms.erase(key);

            auto const lowercase_keysyms =
                sr::views::transform(uppercase_keysyms, [](auto key) { return xkb_keysym_to_lower(key); });
            pressed_keysyms.insert(lowercase_keysyms.begin(), lowercase_keysyms.end());
        }

        pressed_keysyms.erase(keysym);
        pressed_scancodes.erase(scancode);
        return true;
    }

    return false;
}

auto mf::KeyboardStateTracker::keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool
{
    if (!case_insensitive)
        return pressed_keysyms.contains(keysym);

    // Perform case mapping only for a-z and A-Z
    uint32_t lower = keysym;
    uint32_t upper = keysym;

    if (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z)
    {
        lower = xkb_keysym_to_lower(keysym);
    }
    else if (keysym >= XKB_KEY_a && keysym <= XKB_KEY_z)
    {
        upper = xkb_keysym_to_upper(keysym);
    }

    return pressed_keysyms.contains(lower) || pressed_keysyms.contains(upper);
}

auto mf::KeyboardStateTracker::scancode_is_pressed(uint32_t scancode) const -> bool
{
    return pressed_scancodes.contains(scancode);
}

void mf::ActionGroup::add(wayland::Weak<InputTriggerAction const> action)
{
    actions.push_back(action);
    if (timestamp_and_token)
    {
        auto const& [timestamp, token] = timestamp_and_token.value();
        action.value().begin(timestamp, token);
    }
}

auto mf::ActionGroup::any_trigger_active() const -> bool
{
    return timestamp_and_token.has_value();
}

namespace
{
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
}

void mf::ActionGroup::end(std::string const& activation_token, uint32_t wayland_timestamp)
{
    iterate_and_erase_expired(
        actions, [&](auto const& valid_action) { valid_action.end(wayland_timestamp, activation_token); });

    timestamp_and_token = {};
}

void mf::ActionGroup::begin(std::string const& activation_token, uint32_t wayland_timestamp)
{
    timestamp_and_token = {wayland_timestamp, activation_token};

    iterate_and_erase_expired(
        actions, [&](auto const& valid_action) { valid_action.begin(wayland_timestamp, activation_token); });
}

mf::KeyboardTriggerRegistry::KeyboardTriggerRegistry(std::shared_ptr<msh::TokenAuthority> const& token_authority) :
    token_authority{token_authority}
{
}

bool mf::KeyboardTriggerRegistry::register_trigger(mf::KeyboardTrigger* trigger)
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

auto mf::KeyboardTriggerRegistry::matches_any_trigger(MirEvent const& event) -> bool
{
    if (!keyboard_state.process(event))
        return false;

    bool any_matched{false};

    auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());
    iterate_and_erase_expired(
        triggers,
        [&](auto& trigger)
        {
            auto const matched = trigger.process(event, keyboard_state);
            if (matched)
                trigger.begin(*token_authority, timestamp);
            else
                trigger.end(*token_authority, timestamp);

            any_matched |= matched;
        });

    return any_matched;

}

mf::InputTriggerRegistry::InputTriggerRegistry(
    std::shared_ptr<msh::TokenAuthority> const& token_authority, Executor& wayland_executor) :
    token_authority{token_authority},
    wayland_executor{wayland_executor}
{
}

auto mf::InputTriggerRegistry::create_new_action_group() -> std::pair<Token, std::shared_ptr<ActionGroup>>
{
    // Ugly, but we somehow need to tie the action group's lifetime to the
    // token, while using the token to remove the entry in action_groups.

    // Create a pointer to an empty string, not an empty pointer to a string.
    auto token_ptr = std::make_shared<std::string>("");
    auto const ag = std::shared_ptr<mf::ActionGroup>(
        new ActionGroup{},
        [this, token_ptr](auto* action_group)
        {
            action_groups.erase(*token_ptr);
            delete action_group;
        });

    auto const token = token_authority->issue_token(
        [this, ag](auto const& token)
        { wayland_executor.spawn([this, token, ag] { token_revoked(Token{token}); }); });

    *token_ptr = static_cast<std::string>(token);

    action_groups.emplace(token, ag);

    return {static_cast<std::string>(token), ag};
}

void mf::InputTriggerRegistry::token_revoked(Token const& token)
{
    revoked_tokens.add(token);
}

bool mf::InputTriggerRegistry::was_revoked(Token const& token) const
{
    return revoked_tokens.contains(token);
}

auto mf::InputTriggerRegistry::get_action_group(Token const& token) -> std::shared_ptr<ActionGroup> const
{
    if (auto const iter = action_groups.find(token); iter != action_groups.end())
        return iter->second.lock();
    return nullptr;
}
