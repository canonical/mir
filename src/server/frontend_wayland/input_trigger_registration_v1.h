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

namespace mir
{
namespace frontend
{
class InputTriggerRegistry;
class InputTriggerTokenData;

class ActionControl : public wayland::InputTriggerActionControlV1
{
public:
    ActionControl(std::shared_ptr<frontend::InputTriggerTokenData> const& entry, struct wl_resource* id);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

private:
    std::shared_ptr<frontend::InputTriggerTokenData> const entry;
};

auto create_input_trigger_registration_manager_v1(
    wl_display* display, std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>;
}
}
#endif
