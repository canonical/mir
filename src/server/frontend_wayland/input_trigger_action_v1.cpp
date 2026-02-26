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
#include "mir/wayland/weak.h"

#include <mir/wayland/protocol_error.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace
{
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
}

mf::InputTriggerActionV1::InputTriggerActionV1(wl_resource* id) :
    wayland::InputTriggerActionV1{id, Version<1>{}}
{
}

class InputTriggerActionManagerV1 : public mw::InputTriggerActionManagerV1::Global
{
public:
    InputTriggerActionManagerV1(wl_display* display, std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry) :
        Global{display, Version<1>{}},
        input_trigger_registry{input_trigger_registry}
    {
    }

private:
    class Instance : public mir::wayland::InputTriggerActionManagerV1
    {
        std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;

        void get_input_trigger_action(std::string const& token, struct wl_resource* id) override
        {
            if (input_trigger_registry->was_revoked(token))
            {
                new NullInputTriggerActionV1{id}; // Sends `unavailable`
                return;
            }

            if (auto const& action_group = input_trigger_registry->get_action_group(token))
            {
                auto const action = new mf::InputTriggerActionV1 const {id};
                action_group->add(mw::make_weak(action));
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
            wl_resource* new_ext_input_trigger_action_manager_v1, std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry) :
            InputTriggerActionManagerV1{new_ext_input_trigger_action_manager_v1, Version<1>{}},
            input_trigger_registry{input_trigger_registry}
        {
        }
    };

    void bind(wl_resource* new_ext_input_trigger_action_manager_v1) override
    {
        new Instance{new_ext_input_trigger_action_manager_v1, input_trigger_registry};
    }

    std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
};

auto mf::create_input_trigger_action_manager_v1(wl_display* display, std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry)
    -> std::shared_ptr<mw::InputTriggerActionManagerV1::Global>
{
    return std::make_shared<InputTriggerActionManagerV1>(display, input_trigger_registry);
}
