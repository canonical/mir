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

#include "protocol_error.h"

#include "weak.h"

#include <memory>
#include <string>

namespace mf = mir::frontend;
namespace mwrs = mir::wayland_rs;

using ActivationToken = mf::InputTriggerRegistry::ActivationToken;
using Action = mf::InputTriggerRegistry::Action;
using ActionGroupManager = mf::InputTriggerRegistry::ActionGroupManager;

namespace
{

class InputTriggerActionV1 :
    public virtual Action,
    public mwrs::ExtInputTriggerActionV1
{
public:
    InputTriggerActionV1(
        std::shared_ptr<mwrs::Client> client,
        rust::Box<mwrs::ExtInputTriggerActionV1Middleware> instance,
        uint32_t object_id) :
        mwrs::ExtInputTriggerActionV1{std::move(client), std::move(instance), object_id}
    {
    }

    void end(ActivationToken const& activation_token) const override;
    void begin(ActivationToken const& activation_token) const override;
    void unavailable() const override;
};

// Used when a client provides a revoked token to call `send_unavailable_event`.
class NullInputTriggerActionV1 : public mwrs::ExtInputTriggerActionV1
{
public:
    NullInputTriggerActionV1(
        std::shared_ptr<mwrs::Client> client,
        rust::Box<mwrs::ExtInputTriggerActionV1Middleware> instance,
        uint32_t object_id) :
        mwrs::ExtInputTriggerActionV1{std::move(client), std::move(instance), object_id}
    {
        send_unavailable_event();
    }
};

void InputTriggerActionV1::end(ActivationToken const& activation_token) const
{
    const_cast<InputTriggerActionV1*>(this)->send_end_event(
        activation_token.timestamp_ms(), activation_token.token_string());
}

void InputTriggerActionV1::begin(ActivationToken const& activation_token) const
{
    const_cast<InputTriggerActionV1*>(this)->send_begin_event(
        activation_token.timestamp_ms(), activation_token.token_string());
}

void InputTriggerActionV1::unavailable() const
{
    const_cast<InputTriggerActionV1*>(this)->send_unavailable_event();
}
}

mf::InputTriggerActionManagerV1::InputTriggerActionManagerV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ExtInputTriggerActionManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<ActionGroupManager> action_group_manager) :
    mwrs::ExtInputTriggerActionManagerV1{std::move(client), std::move(instance), object_id},
    action_group_manager{std::move(action_group_manager)}
{
}

auto mf::InputTriggerActionManagerV1::get_input_trigger_action(
    rust::String token,
    rust::Box<mwrs::ExtInputTriggerActionV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::ExtInputTriggerActionV1>
{
    std::string const token_str{token};

    if (action_group_manager->was_revoked(token_str))
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_token,
            "ext_input_trigger_action_manager_v1.get_input_trigger_action: trying to use a token (%s) that's no longer valid",
            token_str.c_str()};
    }

    if (auto const& action_group = action_group_manager->get_action_group(token_str))
    {
        if (action_group->was_cancelled())
        {
            return std::make_shared<NullInputTriggerActionV1>(client, std::move(child_instance), child_object_id);
        }

        auto const action = std::make_shared<InputTriggerActionV1>(client, std::move(child_instance), child_object_id);
        action_group->add(mir::wayland_rs::make_weak<Action const>(action.get()));
        return action;
    }

    // Token is not currently valid, nor was it previously revoked. It must be an invalid token.
    throw mwrs::ProtocolError{
        object_id(),
        Error::invalid_token,
        "ext_input_trigger_action_manager_v1.get_input_trigger_action: trying to use a token (%s) we never issued",
        token_str.c_str()};
}
