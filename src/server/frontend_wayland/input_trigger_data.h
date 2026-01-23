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
#include "mir/wayland/weak.h"
#include <algorithm>
#include <deque>
#include <unordered_map>

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

class InputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    InputTriggerActionV1(wl_resource* id) :
        wayland::InputTriggerActionV1{id, Version<1>{}}
    {
    }

    std::vector<std::shared_ptr<input::EventFilter>> trigger_filters;
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

    ~ActionControl() override;

    void add_trigger_pending(wayland::InputTriggerV1 const* trigger);
    void add_trigger_immediate(wayland::InputTriggerV1 const* trigger);
    void drop_trigger_pending(wayland::InputTriggerV1 const* trigger);
    void drop_trigger_immediate(wayland::InputTriggerV1 const* trigger);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

    void install_action(wayland::Weak<frontend::InputTriggerActionV1>);

private:
    std::shared_ptr<mir::Synchronised<frontend::InputTriggerData>> const itd;
    std::shared_ptr<shell::TokenAuthority> const ta;
    std::shared_ptr<input::CompositeEventFilter> const cef;

    // Keeps track of the triggers added or removed while we don't have a valid action
    // If we have an active action, we immediate add or drop the triggers.
    std::unordered_set<wayland::InputTriggerV1 const*> pending_triggers;

    std::string const token;
    wayland::Weak<frontend::InputTriggerActionV1> action;
};

struct InputTriggerData
{
    using Token = std::string;

    InputTriggerData() :
        revoked_tokens{BoundedQueue<Token>{32}}
    {
    }

    std::unordered_map<Token, wayland::Weak<ActionControl>> action_controls;
    std::unordered_map<Token, wayland::Weak<InputTriggerActionV1>> actions;
    BoundedQueue<Token> revoked_tokens;
};

}
}

#endif
