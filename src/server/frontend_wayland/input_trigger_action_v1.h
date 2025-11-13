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

#include "ext-input-trigger-action-v1_wrapper.h"
#include "mir/wayland/weak.h"

namespace mir
{
namespace input
{
}
namespace frontend
{
auto create_input_trigger_action_manager_v1(wl_display*)
    -> std::shared_ptr<wayland::InputTriggerActionManagerV1::Global>;

class InputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    InputTriggerActionV1(wl_resource* id) :
        wayland::InputTriggerActionV1{id, Version<1>{}}
    {
    }
};

extern std::unordered_map<std::string, wayland::Weak<InputTriggerActionV1>> input_trigger_data;
}
}
#endif

