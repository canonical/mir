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

#include "input_trigger_action_v1.h"
#include "input_trigger_registry.h"

#include <mir/wayland/weak.h>
#include <mir/wayland/protocol_error.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using ActivationToken = mf::InputTriggerRegistry::ActivationToken;
using Action = mf::InputTriggerRegistry::Action;
using ActionGroupManager = mf::InputTriggerRegistry::ActionGroupManager;

namespace
{

class InputTriggerActionV1 :
    virtual public Action,
    virtual public mw::InputTriggerActionV1
{
public:
    using mw::InputTriggerActionV1::InputTriggerActionV1;

    void end(ActivationToken const& activation_token) const override;
    void begin(ActivationToken const& activation_token) const override;
    void unavailable() const override;
};

// Used  when a client provides a revoked token to call
// `send_unavailable_event`.
class NullInputTriggerActionV1 : public mw::InputTriggerActionV1
{
public:
    NullInputTriggerActionV1(wl_resource* id) :
        mw::InputTriggerActionV1{id, Version<1>{}}
    {
        send_unavailable_event();
    }
};

class InputTriggerActionManagerV1 : public mw::InputTriggerActionManagerV1::Global
{
public:
    InputTriggerActionManagerV1(wl_display* display, std::shared_ptr<ActionGroupManager> const& action_group_manager) :
        Global{display, Version<1>{}},
        action_group_manager{action_group_manager}
    {
    }

private:
    class Instance : public mir::wayland::InputTriggerActionManagerV1
    {
        std::shared_ptr<ActionGroupManager> const action_group_manager;

        void get_input_trigger_action(std::string const& token, struct wl_resource* id) override
        {
            if (action_group_manager->was_revoked(token))
            {
                BOOST_THROW_EXCEPTION(
                    mw::ProtocolError(
                        resource,
                        Error::invalid_token,
                        "ext_input_trigger_action_manager_v1.get_input_trigger_action: trying to use a token (%s) that's no longer valid",
                        token.c_str()));
            }

            if (auto const& action_group = action_group_manager->get_action_group(token))
            {
                if(action_group->was_cancelled())
                {
                    new NullInputTriggerActionV1{id};
                    return;
                }

                auto* action = new InputTriggerActionV1{id, Version<1>{}};
                action_group->add(mw::make_weak<Action const>(action));
            }
            else
            {
                // Token is not currently valid, nor is was it previously revoked. It must be an invalid token.
                BOOST_THROW_EXCEPTION(
                    mw::ProtocolError(
                        resource,
                        Error::invalid_token,
                        "ext_input_trigger_action_manager_v1.get_input_trigger_action: trying to use a token (%s) we "
                        "never issued",
                        token.c_str()));
            }
        }

    public:
        Instance(
            wl_resource* new_ext_input_trigger_action_manager_v1, std::shared_ptr<ActionGroupManager> const& action_group_manager) :
            InputTriggerActionManagerV1{new_ext_input_trigger_action_manager_v1, Version<1>{}},
            action_group_manager{action_group_manager}
        {
        }
    };

    void bind(wl_resource* new_ext_input_trigger_action_manager_v1) override
    {
        new Instance{new_ext_input_trigger_action_manager_v1, action_group_manager};
    }

    std::shared_ptr<ActionGroupManager> const action_group_manager;
};


void InputTriggerActionV1::end(ActivationToken const& activation_token) const
{
    send_end_event(activation_token.timestamp_ms(), activation_token.token_string());
}

void InputTriggerActionV1::begin(ActivationToken const& activation_token) const
{
    send_begin_event(activation_token.timestamp_ms(), activation_token.token_string());
}

void InputTriggerActionV1::unavailable() const
{
    send_unavailable_event();
}
}

auto mf::create_input_trigger_action_manager_v1(wl_display* display, std::shared_ptr<ActionGroupManager> const& action_group_manager)
    -> std::shared_ptr<mw::InputTriggerActionManagerV1::Global>
{
    return std::make_shared<InputTriggerActionManagerV1>(display, action_group_manager);
}
