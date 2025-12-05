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

#include "ext-input-trigger-registration-v1_wrapper.h"
#include "mir_toolkit/events/enums.h"

namespace mir
{
class Executor;
namespace input
{
class CompositeEventFilter;
}
namespace time
{
class AlarmFactory;
}
namespace frontend
{
class InputTriggerData;
auto create_input_trigger_registration_manager_v1(wl_display*, std::shared_ptr<InputTriggerData> const&)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>;
}

class KeyboardSymTrigger : public wayland::InputTriggerV1
{
public:
    enum class RegistrationType : uint32_t
    {
        tap = wayland::InputTriggerRegistrationManagerV1::RegistrationType::tap,
        hold = wayland::InputTriggerRegistrationManagerV1::RegistrationType::hold,
    };

    KeyboardSymTrigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id, uint32_t type) :
        InputTriggerV1{id, Version<1>{}},
        keysym{keysym},
        type{to_registration_type(type)},
        modifiers{to_mir_modifiers(modifiers, keysym)}
    {
    }

    uint32_t const keysym;
    RegistrationType const type;
    MirInputEventModifiers const modifiers;

private:
    static auto to_registration_type(uint32_t type) -> RegistrationType;
    static auto to_mir_modifiers(uint32_t protocol_modifiers, uint32_t keysym) -> MirInputEventModifiers;
};
}
#endif
