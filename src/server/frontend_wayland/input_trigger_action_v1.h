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
class InputTriggerActionManagerV1Global
{
public:
    virtual ~InputTriggerActionManagerV1Global() = default;
    virtual auto create() -> std::shared_ptr<wayland_rs::ExtInputTriggerActionManagerV1Impl> = 0;
};

auto create_input_trigger_action_manager_v1(
    std::shared_ptr<InputTriggerRegistry::ActionGroupManager> const& action_group_manager)
    -> std::shared_ptr<InputTriggerActionManagerV1Global>;
}
}
#endif
