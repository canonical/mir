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

#include <mir/input/composite_event_filter.h>
#include <mir/input/event_filter.h>
#include <mir/shell/token_authority.h>
#include <mir/wayland/weak.h>

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace frontend
{

// Forces trigger implementations to provide a to_string method for logging
class InputTriggerV1 : public wayland::InputTriggerV1
{
public:
    using wayland::InputTriggerV1::InputTriggerV1;
    virtual auto to_string() const -> std::string = 0;
};

// Forces trigger filters to provide a way to identify the trigger they correspond to
class InputTriggerFilter : public input::EventFilter
{
public:
    virtual auto is_same_trigger(wayland::InputTriggerV1 const* trigger) const -> bool = 0;
};

/// Tracks keyboard state shared among all keyboard event filters
class KeyboardStateTracker
{
public:
    KeyboardStateTracker() = default;

    /// Update pressed keysyms on key down
    void on_key_down(uint32_t keysym, uint32_t scancode);

    /// Update pressed keysyms on key up
    void on_key_up(uint32_t keysym, uint32_t scancode);

    /// Check if a keysym exists in the pressed set
    auto keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool;

    /// Check if a scancode exists in the pressed set
    auto scancode_is_pressed(uint32_t scancode) const -> bool;

    /// Check if protocol modifiers match event modifiers
    static auto protocol_and_event_modifiers_match(uint32_t protocol_modifiers, MirInputEventModifiers event_mods)
        -> bool;

private:
    std::unordered_set<uint32_t> pressed_keysyms;
    std::unordered_set<uint32_t> pressed_scancodes;
};

/// Base class for keyboard triggers with shared modifier logic
class KeyboardTrigger : public frontend::InputTriggerV1
{
public:
    KeyboardTrigger(uint32_t modifiers, struct wl_resource* id);

    /// Check if this trigger matches the current keyboard state
    virtual auto matches(std::shared_ptr<KeyboardStateTracker> const& keyboard_state, MirInputEventModifiers event_mods) const -> bool = 0;

    MirInputEventModifiers const modifiers;

protected:
    static auto to_mir_modifiers(uint32_t protocol_modifiers, uint32_t keysym) -> MirInputEventModifiers;
};

class KeyboardSymTrigger : public KeyboardTrigger
{
public:
    KeyboardSymTrigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id);

    static auto from(wayland::InputTriggerV1* trigger) -> KeyboardSymTrigger*;

    static auto from(wayland::InputTriggerV1 const* trigger) -> KeyboardSymTrigger const*;

    auto to_string() const -> std::string override;

    auto matches(std::shared_ptr<KeyboardStateTracker> const& keyboard_state, MirInputEventModifiers event_mods) const -> bool override;

    uint32_t const keysym;
};

class KeyboardCodeTrigger : public KeyboardTrigger
{
public:
    KeyboardCodeTrigger(uint32_t modifiers, uint32_t scancode, struct wl_resource* id);

    static auto from(wayland::InputTriggerV1* trigger) -> KeyboardCodeTrigger*;

    static auto from(wayland::InputTriggerV1 const* trigger) -> KeyboardCodeTrigger const*;

    auto to_string() const -> std::string override;

    auto matches(std::shared_ptr<KeyboardStateTracker> const& keyboard_state, MirInputEventModifiers event_mods) const -> bool override;

    uint32_t const scancode;
};

struct KeyboardEventFilter : public InputTriggerFilter
{
public:
    explicit KeyboardEventFilter(
        wayland::Weak<wayland::InputTriggerActionV1 const> const& action,
        wayland::Weak<frontend::KeyboardTrigger const> const& trigger,
        std::shared_ptr<shell::TokenAuthority> const& token_authority,
        std::shared_ptr<KeyboardStateTracker> const& keyboard_state);

    bool handle(MirEvent const& event) override;
    auto is_same_trigger(wayland::InputTriggerV1 const* trigger) const -> bool override;

private:
    wayland::Weak<wayland::InputTriggerActionV1 const> const action;
    wayland::Weak<frontend::KeyboardTrigger const> const trigger;
    std::shared_ptr<shell::TokenAuthority> const token_authority;
    std::shared_ptr<KeyboardStateTracker> const keyboard_state;

    bool began{false};
};

class InputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    InputTriggerActionV1(
        std::shared_ptr<shell::TokenAuthority> const& ta,
        std::shared_ptr<input::CompositeEventFilter> const& cef,
        std::shared_ptr<KeyboardStateTracker> const& keyboard_state,
        wl_resource* id);

    static auto dummy(wl_resource* id) -> InputTriggerActionV1*;

    auto has_trigger(wayland::InputTriggerV1 const* trigger) const -> bool;

    void add_trigger(wayland::InputTriggerV1 const* trigger);

    void drop_trigger(wayland::InputTriggerV1 const* trigger);

private:
    InputTriggerActionV1(wl_resource* id);

    std::vector<std::shared_ptr<InputTriggerFilter>> trigger_filters;
    std::shared_ptr<shell::TokenAuthority> const ta;
    std::shared_ptr<input::CompositeEventFilter> const cef;
    std::shared_ptr<KeyboardStateTracker> const keyboard_state;
};

class ActionControl : public wayland::InputTriggerActionControlV1
{
public:
    ActionControl(std::string_view token, struct wl_resource* id);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

    void install_action(wayland::Weak<frontend::InputTriggerActionV1>);

private:
    void add_trigger_pending(wayland::InputTriggerV1 const* trigger);
    void add_trigger_immediate(wayland::InputTriggerV1 const* trigger);
    void drop_trigger_pending(wayland::InputTriggerV1 const* trigger);
    void drop_trigger_immediate(wayland::InputTriggerV1 const* trigger);

    // Keeps track of the triggers added or removed while we don't have a valid action
    // If we have an active action, we immediately add or drop the triggers.
    std::unordered_set<wayland::InputTriggerV1 const*> pending_triggers;

    std::string const token;
    wayland::Weak<frontend::InputTriggerActionV1> action;
};

struct InputTriggerData
{
    using Token = std::string;

    InputTriggerData(
        std::shared_ptr<shell::TokenAuthority> const& ta, std::shared_ptr<input::CompositeEventFilter> const& cef) :
        ta{ta},
        cef{cef},
        revoked_tokens{BoundedQueue<Token>{32}},
        keyboard_state{std::make_shared<KeyboardStateTracker>()}
    {
    }

    auto add_new_action(std::string const& token, struct wl_resource* id) -> bool
    {
        if (revoked_tokens.contains(token))
        {
            auto const action = InputTriggerActionV1::dummy(id);
            action->send_unavailable_event();
            return true;
        }

        if (auto const it = action_controls.find(token); it != action_controls.end())
        {
            auto const action = wayland::make_weak(new InputTriggerActionV1(ta, cef, keyboard_state, id));

            auto& [_, action_control] = *it;
            if (action_control)
                action_control.value().install_action(action);

            actions.insert({token, action});
            return true;
        }

        return false;
    }

    // If the token becomes invalid before an action is associated with
    // it, we should clean up the action control object.
    // When the client tries obtaining the action object using the token,
    // they will get an `unavailable` event.
    void token_revoked(std::string const& token)
    {
        if (!actions.contains(token))
            action_controls.erase(token);

        revoked_tokens.enqueue(token);
    }

    void add_new_action_control(std::string const& token, struct wl_resource* id)
    {
        auto const action_control = wayland::make_weak(new ActionControl{token, id});
        action_controls.insert({token, action_control});
    }

    void erase_expired_entries()
    {
        std::erase_if(actions, [](auto const& pair) { return !pair.second; });
        std::erase_if(action_controls, [](auto const& pair) { return !pair.second; });
    }

    auto has_trigger(wayland::InputTriggerV1 const* trigger) -> bool
    {
        erase_expired_entries();

        // All elements are should be valid now
        return std::ranges::any_of(
            actions, [trigger](auto const pair) { return pair.second.value().has_trigger(trigger); });
    }

private:
    template <typename T> class BoundedQueue
    {
    public:
        explicit BoundedQueue(size_t max_size) :
            max_size{max_size}
        {
        }

        void enqueue(T const& item)
        {
            queue.push_back(item);

            if (queue.size() > max_size)
            {
                queue.pop_front();
            }
        }

        auto contains(T const& item) const -> bool
        {
            return std::find(queue.begin(), queue.end(), item) != queue.end();
        }

        auto size() const -> size_t
        {
            return queue.size();
        }

    private:
        size_t const max_size;
        std::deque<T> queue;
    };

    std::shared_ptr<shell::TokenAuthority> const ta;
    std::shared_ptr<input::CompositeEventFilter> const cef;

    std::unordered_map<Token, wayland::Weak<ActionControl>> action_controls;
    std::unordered_map<Token, wayland::Weak<InputTriggerActionV1>> actions;

    BoundedQueue<Token> revoked_tokens;
    std::shared_ptr<KeyboardStateTracker> keyboard_state;

};
}
}

#endif
