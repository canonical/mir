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
#include "mir/synchronised.h"
#include "mir/wayland/weak.h"
#include <unordered_map>

namespace mir
{
namespace frontend
{

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
        std::shared_ptr<frontend::InputTriggerData> const& itd,
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
    std::shared_ptr<frontend::InputTriggerData> const itd;
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
    mir::Synchronised<std::unordered_map<Token, wayland::Weak<ActionControl>>> action_controls;
    mir::Synchronised<std::unordered_map<Token, wayland::Weak<InputTriggerActionV1>>> actions;
};

}
}

#endif
