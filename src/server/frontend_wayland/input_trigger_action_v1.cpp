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

#include "weak.h"
#include "protocol_error.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;

using ActivationToken = mf::InputTriggerRegistry::ActivationToken;
using Action = mf::InputTriggerRegistry::Action;
using ActionGroupManager = mf::InputTriggerRegistry::ActionGroupManager;

namespace
{

class InputTriggerActionV1 :
    virtual public Action,
    virtual public mw::ExtInputTriggerActionV1Impl
{
public:
    using mw::ExtInputTriggerActionV1Impl::ExtInputTriggerActionV1Impl;

    void end(ActivationToken const& activation_token) override;
    void begin(ActivationToken const& activation_token) override;
    void unavailable() override;
};

// Used  when a client provides a revoked token to call
// `send_unavailable_event`.
class NullInputTriggerActionV1 : public mw::ExtInputTriggerActionV1Impl
{
public:
    NullInputTriggerActionV1()
    {
    }

    auto associate(rust::Box<mir::wayland_rs::ExtInputTriggerActionV1Ext> instance, uint32_t object_id) -> void override
    {
        mw::ExtInputTriggerActionV1Impl::associate(std::move(instance), object_id);
        send_unavailable_event();
    }
};

class InputTriggerActionManagerV1 : public mf::InputTriggerActionManagerV1Global
{
public:
    InputTriggerActionManagerV1(std::shared_ptr<ActionGroupManager> const& action_group_manager) :
        action_group_manager{action_group_manager}
    {
    }

private:
    class Instance : public mir::wayland_rs::ExtInputTriggerActionManagerV1Impl
    {
        std::shared_ptr<ActionGroupManager> const action_group_manager;

        auto get_input_trigger_action(rust::String token) -> std::shared_ptr<mir::wayland_rs::ExtInputTriggerActionV1Impl> override
        {
            if (action_group_manager->was_revoked(token.c_str()))
            {
                BOOST_THROW_EXCEPTION(
                    mw::ProtocolError(
                        object_id(),
                        Error::invalid_token,
                        "ext_input_trigger_action_manager_v1.get_input_trigger_action: trying to use a token (%s) that's no longer valid",
                        token.c_str()));
            }

            if (auto const& action_group = action_group_manager->get_action_group(token.c_str()))
            {
                if(action_group->was_cancelled())
                {
                    return std::make_shared<NullInputTriggerActionV1>();
                }

                auto const action = std::make_shared<InputTriggerActionV1>();
                action_group->add(mw::Weak<Action const>(action));
                return action;
            }
            else
            {
                // Token is not currently valid, nor was it previously revoked. It must be an invalid token.
                BOOST_THROW_EXCEPTION(
                    mw::ProtocolError(
                        object_id(),
                        Error::invalid_token,
                        "ext_input_trigger_action_manager_v1.get_input_trigger_action: trying to use a token (%s) we "
                        "never issued",
                        token.c_str()));
            }
        }

    public:
        Instance(
            std::shared_ptr<ActionGroupManager> const& action_group_manager) :
            action_group_manager{action_group_manager}
        {
        }
    };

    auto create() -> std::shared_ptr<mir::wayland_rs::ExtInputTriggerActionManagerV1Impl> override
    {
        return std::make_shared<Instance>(action_group_manager);
    }

    std::shared_ptr<ActionGroupManager> const action_group_manager;
};


void InputTriggerActionV1::end(ActivationToken const& activation_token)
{
    send_end_event(activation_token.timestamp_ms(), activation_token.token_string());
}

void InputTriggerActionV1::begin(ActivationToken const& activation_token)
{
    send_begin_event(activation_token.timestamp_ms(), activation_token.token_string());
}

void InputTriggerActionV1::unavailable()
{
    send_unavailable_event();
}
}

auto mf::create_input_trigger_action_manager_v1(std::shared_ptr<ActionGroupManager> const& action_group_manager)
    -> std::shared_ptr<mf::InputTriggerActionManagerV1Global>
{
    return std::make_shared<InputTriggerActionManagerV1>(action_group_manager);
}
