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

#include "input_trigger_registration_v1.h"

#include "input_trigger_registry.h"
#include "input_trigger_v1.h"

#include <mir/log.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace
{
class InputTriggerActionControlV1 : public mw::InputTriggerActionControlV1
{
public:
    InputTriggerActionControlV1(std::shared_ptr<mf::ActionGroup> const& action_group, struct wl_resource* id);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

private:
    std::shared_ptr<mf::ActionGroup> const action_group;
};

InputTriggerActionControlV1::InputTriggerActionControlV1(std::shared_ptr<mf::ActionGroup> const& action_group, struct wl_resource* id) :
    mw::InputTriggerActionControlV1{id, Version<1>{}},
    action_group{action_group}
{
}

void InputTriggerActionControlV1::add_input_trigger_event(struct wl_resource* trigger)
{
    if (auto* input_trigger = mf::InputTriggerV1::from(trigger))
    {
        input_trigger->associate_with_action_group(action_group);
    }
}

void InputTriggerActionControlV1::drop_input_trigger_event(struct wl_resource* trigger_id)
{
    if (auto* input_trigger = mf::InputTriggerV1::from(trigger_id))
    {
        input_trigger->unassociate_with_action_group(action_group);
    }
}

class InputTriggerRegistrationManagerV1 : public mw::InputTriggerRegistrationManagerV1::Global
{
public:
    InputTriggerRegistrationManagerV1(
        wl_display* display, std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry) :
        Global{display, Version<1>{}},
        input_trigger_registry{input_trigger_registry}
    {
    }

    class Instance : public mw::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry) :
            mw::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            input_trigger_registry{input_trigger_registry}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, input_trigger_registry};
    }

private:
    std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
};

// Triggers are stored in the action control object until the corresponding
// action is obtained by the client. In that state, they are "inactive" and are
// not checked for matching or duplicates.
//
// Once a client obtains the action, the triggers become "active" and are
// checked for matching and duplicates.
void InputTriggerRegistrationManagerV1::Instance::register_keyboard_sym_trigger(
    uint32_t modifiers, uint32_t keysym, struct wl_resource* id)
{
    auto const shift_adjustment = (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z);
    auto* keyboard_trigger =
        new mf::KeyboardSymTrigger{mf::InputTriggerModifiers::from_protocol(modifiers, shift_adjustment), keysym, id};

    if (!input_trigger_registry->register_trigger(keyboard_trigger))
    {
        mir::log_warning("register_keyboard_sym_trigger: %s already registered", keyboard_trigger->to_c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_code_trigger(
    uint32_t modifiers, uint32_t keycode, struct wl_resource* id)
{
    auto* keyboard_trigger =
        new mf::KeyboardCodeTrigger{mf::InputTriggerModifiers::from_protocol(modifiers), keycode, id};

    if (!input_trigger_registry->register_trigger(keyboard_trigger))
    {
        mir::log_warning("register_keyboard_code_trigger: %s already registered", keyboard_trigger->to_c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    auto const [token, action_group] = input_trigger_registry->create_new_action_group();
    auto const action_control = new InputTriggerActionControlV1{action_group, id};
    action_control->send_done_event(token);
}
}

auto mf::create_input_trigger_registration_manager_v1(wl_display* display, std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry)
    -> std::shared_ptr<mw::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<InputTriggerRegistrationManagerV1>(display, input_trigger_registry);
}
