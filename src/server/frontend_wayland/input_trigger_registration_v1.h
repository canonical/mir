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

#include "ext_input_trigger_registration_v1.h"
#include "input_trigger_registry.h"

#include <memory>

namespace mir
{
namespace frontend
{
class KeyboardStateTracker;

class InputTriggerRegistrationManagerV1 : public wayland::ExtInputTriggerRegistrationManagerV1
{
public:
    InputTriggerRegistrationManagerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::ExtInputTriggerRegistrationManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<InputTriggerRegistry::ActionGroupManager> action_group_manager,
        std::shared_ptr<InputTriggerRegistry> input_trigger_registry,
        std::shared_ptr<KeyboardStateTracker> keyboard_state_tracker);

private:
    auto register_keyboard_sym_trigger(
        uint32_t modifiers,
        uint32_t keysym,
        rust::Box<wayland::ExtInputTriggerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ExtInputTriggerV1> override;

    auto register_keyboard_code_trigger(
        uint32_t modifiers,
        uint32_t keycode,
        rust::Box<wayland::ExtInputTriggerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ExtInputTriggerV1> override;

    auto get_action_control(
        rust::String name,
        rust::Box<wayland::ExtInputTriggerActionControlV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ExtInputTriggerActionControlV1> override;

    std::shared_ptr<InputTriggerRegistry::ActionGroupManager> const action_group_manager;
    std::shared_ptr<InputTriggerRegistry> const input_trigger_registry;
    std::shared_ptr<KeyboardStateTracker> const keyboard_state_tracker;
};
}
}
#endif
