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
#include "input_trigger_action_v1.h"
#include "input_trigger_data.h"

#include <mir/executor.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/event_filter.h>
#include <mir/input/xkb_mapper.h>
#include <mir/log.h>
#include <mir/shell/token_authority.h>
#include <mir/synchronised.h>
#include <mir/wayland/weak.h>

namespace mf = mir::frontend;
namespace mi = mir::input;
namespace msh = mir::shell;

namespace mir
{
namespace frontend
{

class InputTriggerRegistrationManagerV1 : public wayland::InputTriggerRegistrationManagerV1::Global
{
public:
    InputTriggerRegistrationManagerV1(
        wl_display* display,
        std::shared_ptr<InputTriggerData> const& itd) :
        Global{display, Version<1>{}},
        itd{itd}
    {
    }

    class Instance : public wayland::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<InputTriggerData> const& itd) :
            wayland::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            itd{itd}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        auto has_trigger(wayland::InputTriggerV1 const*) const -> bool;

        std::shared_ptr<InputTriggerData> const itd;
        std::shared_ptr<msh::TokenAuthority> const ta;
        std::shared_ptr<input::CompositeEventFilter> const cef;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, itd};
    }

private:
    std::shared_ptr<InputTriggerData> const itd;
};

// The filter is corresponding to this trigger is registered with the composite
// event filter when the client gets the owning action. Which requires the
// client to obtain an action control object, add add triggers to it, and then
// get the action using the token provided by the action control object.
void InputTriggerRegistrationManagerV1::Instance::register_keyboard_sym_trigger(
    uint32_t modifiers, uint32_t keysym, struct wl_resource* id)
{
    auto const* keyboard_trigger = new KeyboardSymTrigger{modifiers, keysym, id};

    if(has_trigger(keyboard_trigger))
    {
        mir::log_error("%s already registered", keyboard_trigger->to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_code_trigger(
    uint32_t modifiers, uint32_t keycode, struct wl_resource* id)
{
    auto const* keyboard_trigger = new KeyboardCodeTrigger{modifiers, keycode, id};

    if(has_trigger(keyboard_trigger))
    {
        mir::log_error("%s already registered", keyboard_trigger->to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    auto const token = ta->issue_token(
        [itd = itd](auto const& token)
        {
            itd->token_revoked(std::string{token});
        });

    auto const token_string = static_cast<std::string>(token);
    itd->add_new_action_control(token_string, id);

}

auto InputTriggerRegistrationManagerV1::Instance::has_trigger(wayland::InputTriggerV1 const* trigger) const -> bool
{
    return itd->has_trigger(trigger);
}

auto create_input_trigger_registration_manager_v1(
    wl_display* display,
    std::shared_ptr<InputTriggerData> const& itd)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<mf::InputTriggerRegistrationManagerV1>(display, itd);
}

}
}
