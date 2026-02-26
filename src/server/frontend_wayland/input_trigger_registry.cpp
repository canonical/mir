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
#include "input_trigger_action_v1.h"
#include "input_trigger_v1.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/input/xkb_mapper.h>
#include <mir/log.h>
#include <mir_toolkit/events/enums.h>

#include <wayland-server-core.h>

#include <algorithm>
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

void mf::KeyboardStateTracker::on_key_down(uint32_t keysym, uint32_t scancode)
{
    pressed_keysyms.insert(keysym);
    pressed_scancodes.insert(scancode);
}

void mf::KeyboardStateTracker::on_key_up(uint32_t keysym, uint32_t scancode)
{
    pressed_keysyms.erase(keysym);
    pressed_scancodes.erase(scancode);
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
        lower = keysym + (XKB_KEY_a - XKB_KEY_A);
    }
    else if (keysym >= XKB_KEY_a && keysym <= XKB_KEY_z)
    {
        upper = keysym - (XKB_KEY_a - XKB_KEY_A);
    }

    return pressed_keysyms.contains(lower) || pressed_keysyms.contains(upper);
}

auto mf::KeyboardStateTracker::scancode_is_pressed(uint32_t scancode) const -> bool
{
    return pressed_scancodes.contains(scancode);
}


void mf::InputTriggerTokenData::ActionGroup::add(wayland::Weak<frontend::InputTriggerActionV1 const> action)
{
    actions.push_back(action);
}

auto mf::InputTriggerTokenData::ActionGroup::began() const -> bool
{
    return began_;
}

namespace
{
void iterate_and_erase_expired_actions(
    std::vector<mir::wayland::Weak<mf::InputTriggerActionV1 const>>& actions, auto&& callback)
{
    std::erase_if(
        actions,
        [&](auto const& action)
        {
            if (!action)
                return true; // Erase

            callback(action.value());
            return false; // Don't erase
        });
}
}

void mf::InputTriggerTokenData::ActionGroup::end(std::string const& activation_token, uint32_t wayland_timestamp)
{
    iterate_and_erase_expired_actions(
        actions, [&](auto const& valid_action) { valid_action.send_end_event(wayland_timestamp, activation_token); });

    began_ = false;
}

void mf::InputTriggerTokenData::ActionGroup::begin(std::string const& activation_token, uint32_t wayland_timestamp)
{

    iterate_and_erase_expired_actions(
        actions, [&](auto const& valid_action) { valid_action.send_begin_event(wayland_timestamp, activation_token); });

    began_ = true;
}

bool mf::InputTriggerTokenData::ActionGroup::empty() const
{
    return actions.empty();
}

void mf::InputTriggerTokenData::add_action(wayland::Weak<frontend::InputTriggerActionV1 const> action)
{
    action_group.add(action);
    trigger_list.insert(trigger_list.end(), pending_triggers.begin(), pending_triggers.end());
    pending_triggers.clear();
}

void mf::InputTriggerTokenData::add_trigger(wayland::Weak<frontend::InputTriggerV1 const> trigger)
{
    if (action_group.empty())
        pending_triggers.push_back(trigger);
    else
        trigger_list.push_back(trigger);
}

void mf::InputTriggerTokenData::drop_trigger(wayland::Weak<frontend::InputTriggerV1 const> trigger)
{
    if (action_group.empty())
        std::erase(pending_triggers, trigger);
    else
        std::erase(trigger_list, trigger);
}

bool mf::InputTriggerTokenData::has_trigger(frontend::InputTriggerV1 const* trigger) const
{
    auto const has_in_list = [trigger](auto const& list)
    {
        return sr::any_of(
            list,
            [trigger](auto const& t) { return t.value().is_same_trigger(trigger); });
    };

    return has_in_list(trigger_list) || has_in_list(pending_triggers);
}

bool mf::InputTriggerTokenData::matches(MirEvent const& event, KeyboardStateTracker const& keyboard_state) const
{
    return sr::any_of(
        trigger_list,
        [&](auto const& trigger) { return trigger.value().matches(event, keyboard_state); });
}

bool mf::InputTriggerTokenData::began() const
{
    return action_group.began();
}

void mf::InputTriggerTokenData::begin(std::string const& activation_token, uint32_t wayland_timestamp)
{
    action_group.begin(activation_token, wayland_timestamp);
}

void mf::InputTriggerTokenData::end(std::string const& activation_token, uint32_t wayland_timestamp)
{
    action_group.end(activation_token, wayland_timestamp);
}

mf::InputTriggerRegistry::InputTriggerRegistry(
    std::shared_ptr<msh::TokenAuthority> const& token_authority, Executor& wayland_executor) :
    token_authority{token_authority},
    wayland_executor{wayland_executor}
{
}

auto mf::InputTriggerRegistry::create_new_token_data() -> std::pair<Token, std::shared_ptr<InputTriggerTokenData>>
{
    auto const td = std::make_shared<mf::InputTriggerTokenData>();

    // An entry's lifetime is tied to three things: the token it is associated
    // with, the action control object, and any action objects that may be
    // created for it.
    //
    // Here we capture a pointer to the entry which will _at least_ keep it
    // alive until the token is revoked
    auto const token = token_authority->issue_token(
        [this, td](auto const& token)
        { wayland_executor.spawn([this, token, td] { token_revoked(Token{token}); }); });

    token_data.emplace(token, td);

    return {static_cast<std::string>(token), td};
}

auto mf::InputTriggerRegistry::has_trigger(mf::InputTriggerV1 const* trigger) -> bool
{
    return sr::any_of(
        token_data,
        [trigger](auto const& pair)
        {
            auto const& entry_weak = pair.second;
            if (auto entry = entry_weak.lock())
                return entry->has_trigger(trigger);
            return false;
        });
}

void mf::InputTriggerRegistry::token_revoked(Token const& token)
{
    revoked_tokens.add(token);
}

bool mf::InputTriggerRegistry::matches_any_trigger(MirEvent const& event)
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
        keyboard_state.on_key_down(keysym, scancode);
    else if (action == mir_keyboard_action_up)
        keyboard_state.on_key_up(keysym, scancode);
    else
        return false;

    bool any_matched{false};

    for (auto it = token_data.begin(); it != token_data.end();)
    {
        auto e = it->second.lock();
        if (!e)
        {
            it = token_data.erase(it);
            continue;
        }

        auto const matched = e->matches(event, keyboard_state);

        if (!matched)
        {
            if (e->began())
            {
                auto const activation_token = std::string{token_authority->issue_token(std::nullopt)};
                auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());
                e->end(activation_token, timestamp);
            }
            ++it;
            continue;
        }

        if (!e->began())
        {
            auto const activation_token = std::string{token_authority->issue_token(std::nullopt)};
            auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());
            e->begin(activation_token, timestamp);
            any_matched = true;
            ++it;
            continue;
        }
        ++it;
    }

    return any_matched;
}

bool mf::InputTriggerRegistry::was_revoked(Token const& token) const
{
    return revoked_tokens.contains(token);
}

auto mf::InputTriggerRegistry::get_token_data(Token const& token) -> std::shared_ptr<InputTriggerTokenData> const
{
    if (auto const iter = token_data.find(token); iter != token_data.end())
        return iter->second.lock();
    return nullptr;
}
