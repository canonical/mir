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

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_ACTION_V1_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_ACTION_V1_H_

#include "ext_input_trigger_action_v1.h"
#include "input_trigger_registry.h"

#include <memory>

namespace mir
{
namespace frontend
{
class InputTriggerActionManagerV1 : public wayland_rs::ExtInputTriggerActionManagerV1
{
public:
    InputTriggerActionManagerV1(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::ExtInputTriggerActionManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<InputTriggerRegistry::ActionGroupManager> action_group_manager);

private:
    auto get_input_trigger_action(
        rust::String token,
        rust::Box<wayland_rs::ExtInputTriggerActionV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::ExtInputTriggerActionV1> override;

    std::shared_ptr<InputTriggerRegistry::ActionGroupManager> const action_group_manager;
};
}
}
#endif
