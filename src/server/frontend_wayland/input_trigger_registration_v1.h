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

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_REGISTRATION_V1_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_REGISTRATION_V1_H_

#include "input_trigger_registry.h"
#include "wayland_rs/wayland_rs_cpp/include/ext_input_trigger_registration_v1.h"

namespace mir
{
namespace frontend
{
class KeyboardStateTracker;

auto create_input_trigger_registration_manager_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    std::shared_ptr<InputTriggerRegistry::ActionGroupManager> const& action_group_manager,
    std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry,
    std::shared_ptr<KeyboardStateTracker> const& keyboard_state_tracker)
    -> std::shared_ptr<wayland_rs::ExtInputTriggerRegistrationManagerV1Impl>;
}
}
#endif
