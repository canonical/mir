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

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/executor.h>
#include <mir/input/xkb_mapper.h>
#include <mir/wayland/weak.h>
#include <mir/input/event_filter.h>
#include <mir/input/composite_event_filter.h>

#include "mir_toolkit/events/enums.h"

namespace mf = mir::frontend;
namespace mi = mir::input;

namespace mir
{
namespace frontend
{

class InputTriggerRegistrationManagerV1 : public wayland::InputTriggerRegistrationManagerV1::Global
{
public:
    InputTriggerRegistrationManagerV1(wl_display* display, std::shared_ptr<InputTriggerData> const& itd) :
        Global{display, Version<1>{}},
        itd{itd}
    {
    }

    class Instance : public wayland::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(wl_resource* new_ext_input_trigger_registration_manager_v1, std::shared_ptr<InputTriggerData> const& itd) :
            wayland::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            itd{itd}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        std::shared_ptr<InputTriggerData> const itd;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, itd};
    }

private:
    std::shared_ptr<InputTriggerData> const itd;
};

class ActionControl : public wayland::InputTriggerActionControlV1
{
public:
    ActionControl(std::shared_ptr<InputTriggerData> const& itd, struct wl_resource* id) :
        mir::wayland::InputTriggerActionControlV1{id, Version<1>{}},
        itd{itd}
    {
    }

    void add_input_trigger_event(struct wl_resource* trigger) override
    {
        if(auto const input_trigger = wayland::InputTriggerV1::from(trigger))
        {
            auto const token = "foo";
            itd->pending_actions.lock()->emplace(token, wayland::make_weak(input_trigger));

            // Tell the client that the action has been successfully registered.
            // They then should call
            // `InputTriggerActionManagerV1::get_input_trigger_action` using the
            // token we supply here.
            send_done_event(token);
        }
    }
    virtual void drop_input_trigger_event(struct wl_resource* /*trigger*/) override
    {
    }

private:
    std::shared_ptr<InputTriggerData> const itd;
    std::shared_ptr<mi::EventFilter> trigger_filter;
};


// The trigger is registered with the composite event filter when its added to a control object
void InputTriggerRegistrationManagerV1::Instance::register_keyboard_sym_trigger(
    uint32_t modifiers, uint32_t keysym, struct wl_resource* id)
{
    auto const* keyboard_trigger = new KeyboardSymTrigger{modifiers, keysym, id};

    // TODO validation before done event
    keyboard_trigger->send_done_event();
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_code_trigger(
    uint32_t, uint32_t, struct wl_resource*)
{
}

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    auto action_control = new ActionControl{itd, id};
    action_control->send_done_event("foo");
}
}

auto KeyboardSymTrigger::to_mir_modifiers(uint32_t protocol_modifiers, uint32_t keysym) -> MirInputEventModifiers
{
    using PM = wayland::InputTriggerRegistrationManagerV1::Modifiers;

    if (protocol_modifiers == 0)
        return mir_input_event_modifier_none;

    struct Mapping
    {
        uint32_t protocol_modifier;
        MirInputEventModifiers mir_modifier;
    };

    // clang-format off
        constexpr std::array<std::pair<uint32_t, MirInputEventModifiers>, 14> mappings = {
            std::pair{ PM::alt,         mir_input_event_modifier_alt },
            std::pair{ PM::alt_left,    MirInputEventModifiers(mir_input_event_modifier_alt_left | mir_input_event_modifier_alt) },
            std::pair{ PM::alt_right,   MirInputEventModifiers(mir_input_event_modifier_alt_right | mir_input_event_modifier_alt) },

            std::pair{ PM::shift,       mir_input_event_modifier_shift },
            std::pair{ PM::shift_left,  MirInputEventModifiers(mir_input_event_modifier_shift_left | mir_input_event_modifier_shift) },
            std::pair{ PM::shift_right, MirInputEventModifiers(mir_input_event_modifier_shift_right | mir_input_event_modifier_shift) },

            std::pair{ PM::sym,         mir_input_event_modifier_sym },
            std::pair{ PM::function,    mir_input_event_modifier_function },

            std::pair{ PM::ctrl,        mir_input_event_modifier_ctrl },
            std::pair{ PM::ctrl_left,   MirInputEventModifiers(mir_input_event_modifier_ctrl_left | mir_input_event_modifier_ctrl) },
            std::pair{ PM::ctrl_right,  MirInputEventModifiers(mir_input_event_modifier_ctrl_right | mir_input_event_modifier_ctrl) },

            std::pair{ PM::meta,        mir_input_event_modifier_meta },
            std::pair{ PM::meta_left,   MirInputEventModifiers(mir_input_event_modifier_meta_left | mir_input_event_modifier_meta) },
            std::pair{ PM::meta_right,  MirInputEventModifiers(mir_input_event_modifier_meta_right | mir_input_event_modifier_meta) },
        };
    // clang-format on

    MirInputEventModifiers result = 0;
    for (auto const& [protocol_modifier, mir_modifier] : mappings)
    {
        if (protocol_modifiers & protocol_modifier)
            result |= mir_modifier;
    }

    if (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z)
        result |= mir_input_event_modifier_shift;

    return result;
}
}

auto mf::create_input_trigger_registration_manager_v1(wl_display* display, std::shared_ptr<InputTriggerData> const& itd)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<mf::InputTriggerRegistrationManagerV1>(display, itd);
}
