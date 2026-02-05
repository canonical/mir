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

#include "input_trigger_data.h"

#include <mir/log.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

class InputTriggerRegistrationManagerV1 : public mw::InputTriggerRegistrationManagerV1::Global
{
public:
    InputTriggerRegistrationManagerV1(wl_display* display, std::shared_ptr<mf::InputTriggerData> const& itd) :
        Global{display, Version<1>{}},
        itd{itd}
    {
    }

    class Instance : public mw::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<mf::InputTriggerData> const& itd) :
            mw::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            itd{itd}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        auto has_trigger(mw::InputTriggerV1 const*) const -> bool;

        std::shared_ptr<mf::InputTriggerData> const itd;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, itd};
    }

private:
    std::shared_ptr<mf::InputTriggerData> const itd;
};

// Event filters are not immediately added to the composite event filter. They
// are added after the trigger is added to an action control object, and the
// corresponding action is obtained by the client.
void InputTriggerRegistrationManagerV1::Instance::register_keyboard_sym_trigger(
    uint32_t modifiers, uint32_t keysym, struct wl_resource* id)
{
    auto const shift_adjustment = (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z);
    auto const* keyboard_trigger =
        new mf::KeyboardSymTrigger{mf::InputTriggerModifiers::from_protocol(modifiers, shift_adjustment), keysym, id};

    if (has_trigger(keyboard_trigger))
    {
        mir::log_error("register_keyboard_sym_trigger: %s already registered", keyboard_trigger->to_c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_code_trigger(
    uint32_t modifiers, uint32_t keycode, struct wl_resource* id)
{
    auto const* keyboard_trigger =
        new mf::KeyboardCodeTrigger{mf::InputTriggerModifiers::from_protocol(modifiers), keycode, id};

    if (has_trigger(keyboard_trigger))
    {
        mir::log_error("register_keyboard_code_trigger: %s already registered", keyboard_trigger->to_c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    itd->add_new_action_control(id);
}

auto InputTriggerRegistrationManagerV1::Instance::has_trigger(mw::InputTriggerV1 const* trigger) const -> bool
{
    return itd->has_trigger(trigger);
}

auto mf::create_input_trigger_registration_manager_v1(wl_display* display, std::shared_ptr<InputTriggerData> const& itd)
    -> std::shared_ptr<mw::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<InputTriggerRegistrationManagerV1>(display, itd);
}
