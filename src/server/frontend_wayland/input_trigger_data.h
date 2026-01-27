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
#include "mir/input/composite_event_filter.h"

#include <mir/input/event_filter.h>
#include <mir/wayland/weak.h>

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace frontend
{

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
    void on_key_down(uint32_t keysym);

    /// Update pressed keysyms on key up
    void on_key_up(uint32_t keysym);

    /// Check if a keysym exists in the pressed set
    auto keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool;

    /// Check if protocol modifiers match event modifiers
    static auto protocol_and_event_modifiers_match(uint32_t protocol_modifiers, MirInputEventModifiers event_mods)
        -> bool;

private:
    std::unordered_set<uint32_t> pressed_keysyms;
};


class KeyboardSymTrigger : public frontend::InputTriggerV1
{
public:
    KeyboardSymTrigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id);

    static auto from(wayland::InputTriggerV1* trigger) -> KeyboardSymTrigger*;

    static auto from(wayland::InputTriggerV1 const* trigger) -> KeyboardSymTrigger const*;

    auto to_string() const -> std::string override;

    uint32_t const keysym;
    MirInputEventModifiers const modifiers;

private:
    static auto to_mir_modifiers(uint32_t protocol_modifiers, uint32_t keysym) -> MirInputEventModifiers;
};

struct KeyboardEventFilter : public InputTriggerFilter
{
public:
    explicit KeyboardEventFilter(
        wayland::Weak<wayland::InputTriggerActionV1 const> const& action,
        wayland::Weak<frontend::KeyboardSymTrigger const> const& trigger,
        std::shared_ptr<shell::TokenAuthority> const& token_authority,
        std::shared_ptr<KeyboardStateTracker> const& keyboard_state);

    bool handle(MirEvent const& event) override;
    auto is_same_trigger(wayland::InputTriggerV1 const* trigger) const -> bool override;

private:
    wayland::Weak<wayland::InputTriggerActionV1 const> const action;
    wayland::Weak<frontend::KeyboardSymTrigger const> const trigger;
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
    ActionControl(
        std::shared_ptr<mir::Synchronised<frontend::InputTriggerData>> const& itd,
        std::shared_ptr<shell::TokenAuthority> const& ta,
        std::shared_ptr<input::CompositeEventFilter> const& cef,
        std::string_view token,
        struct wl_resource* id);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

    void install_action(wayland::Weak<frontend::InputTriggerActionV1>);

private:
    void add_trigger_pending(wayland::InputTriggerV1 const* trigger);
    void add_trigger_immediate(wayland::InputTriggerV1 const* trigger);
    void drop_trigger_pending(wayland::InputTriggerV1 const* trigger);
    void drop_trigger_immediate(wayland::InputTriggerV1 const* trigger);

    std::shared_ptr<mir::Synchronised<frontend::InputTriggerData>> const itd;
    std::shared_ptr<shell::TokenAuthority> const ta;
    std::shared_ptr<input::CompositeEventFilter> const cef;

    // Keeps track of the triggers added or removed while we don't have a valid action
    // If we have an active action, we immediately add or drop the triggers.
    std::unordered_set<wayland::InputTriggerV1 const*> pending_triggers;

    std::string const token;
    wayland::Weak<frontend::InputTriggerActionV1> action;
};

struct InputTriggerData
{
    using Token = std::string;

    InputTriggerData() :
        revoked_tokens{BoundedQueue<Token>{32}},
        keyboard_state{std::make_shared<KeyboardStateTracker>()}
    {
    }

    std::unordered_map<Token, wayland::Weak<ActionControl>> action_controls;
    std::unordered_map<Token, wayland::Weak<InputTriggerActionV1>> actions;
    BoundedQueue<Token> revoked_tokens;
    std::shared_ptr<KeyboardStateTracker> keyboard_state;
};

}
}

#endif
