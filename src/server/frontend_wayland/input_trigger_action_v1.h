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

#include <memory>

namespace mir
{
namespace frontend
{
class InputTriggerData;

class InputTriggerTokenData;
class InputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    InputTriggerActionV1(std::shared_ptr<InputTriggerTokenData> const& token_data, wl_resource* id);

private:
    std::shared_ptr<InputTriggerTokenData> const token_data;
};

// Used  when a client provides a revoked token to call
// `send_unavailable_event`.
class NullInputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    NullInputTriggerActionV1(wl_resource* id) :
        wayland::InputTriggerActionV1{id, Version<1>{}}
    {
        send_unavailable_event();
    }
};

auto create_input_trigger_action_manager_v1(wl_display*, std::shared_ptr<InputTriggerData> const& itd)
    -> std::shared_ptr<wayland::InputTriggerActionManagerV1::Global>;
}
}
#endif
