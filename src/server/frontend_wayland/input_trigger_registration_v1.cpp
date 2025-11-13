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
#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/wayland/weak.h"
#include "mir_toolkit/events/enums.h"

#include <mir/input/composite_event_filter.h>

namespace mf = mir::frontend;
namespace mi = mir::input;

namespace mir
{
namespace frontend
{
class InputTriggerRegistrationManagerV1 : public wayland::InputTriggerRegistrationManagerV1::Global
{
public:
    InputTriggerRegistrationManagerV1(wl_display* display, std::shared_ptr<mi::CompositeEventFilter> const& cef) :
        Global{display, Version<1>{}},
        cef{cef}
    {
    }

    class Instance : public wayland::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<mi::CompositeEventFilter> const& cef) :
            wayland::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            cef{cef}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void register_modifier_hold_trigger(uint32_t modifiers, struct wl_resource* id) override;
        void register_modifier_tap_trigger(uint32_t modifier, uint32_t tap_count, struct wl_resource* id) override;
        void register_pointer_trigger(uint32_t edges, struct wl_resource* id) override;
        void register_touch_drag_trigger(uint32_t touches, uint32_t direction, struct wl_resource* id) override;
        void register_touch_tap_trigger(uint32_t touches, uint32_t hold_delay, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        std::shared_ptr<mi::CompositeEventFilter> const cef;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, cef};
    }

private:
    std::shared_ptr<mi::CompositeEventFilter> const cef;
};

class KeyboardSymTrigger : public wayland::InputTriggerV1
{
public:
    KeyboardSymTrigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) :
        InputTriggerV1{id, Version<1>{}},
        modifiers{to_mir_modifiers(modifiers)},
        keysym{keysym}
    {
    }

    MirInputEventModifiers const modifiers;
    uint32_t const keysym;

    struct ExtraData
    {
        std::shared_ptr<mi::EventFilter> event_filter;
        std::string action_token;
    } extra_data;

    static auto to_mir_modifiers(uint32_t protocol_modifiers) -> MirInputEventModifiers
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

        return result;
    }
};

class ActionControl : public wayland::InputTriggerActionControlV1
{
public:
    ActionControl(std::shared_ptr<mi::CompositeEventFilter> const& cef, struct wl_resource* id) :
        mir::wayland::InputTriggerActionControlV1{id, Version<1>{}},
        cef{cef}
    {
    }
    void add_input_trigger_event(struct wl_resource* trigger) override
    {

        if(auto const keyboard_trigger = static_cast<KeyboardSymTrigger*>(wayland::InputTriggerV1::from(trigger)))
        {
            auto const token = "foo";

            auto const filter = keyboard_trigger->extra_data.event_filter =
                std::make_shared<KeyboardEventFilter>(wayland::make_weak(keyboard_trigger), token);
            keyboard_trigger->extra_data.action_token = token;

            cef->prepend(filter);
            send_done_event(token);
        }
    }
    virtual void drop_input_trigger_event(struct wl_resource* /*trigger*/) override
    {
    }

private:
    struct KeyboardEventFilter : public mi::EventFilter
    {
        wayland::Weak<KeyboardSymTrigger> const trigger;
        std::string const token;

        bool began{false};

        explicit KeyboardEventFilter(wayland::Weak<KeyboardSymTrigger> trigger, std::string const& token) :
            trigger{std::move(trigger)},
            token{token}
        {
        }

        bool handle(MirEvent const& event) override
        {
            if (event.type() != mir_event_type_input)
                return false;

            auto const* input_event = event.to_input();
            if (input_event->input_type() != mir_input_event_type_key)
                return false;

            auto const* key_event = input_event->to_keyboard();

            if (auto const action_iter = input_trigger_data.find(trigger.value().extra_data.action_token);
                action_iter != input_trigger_data.end())
            {
                auto const [_, action] = *action_iter;
                // TODO pass the clock and the token authority
                auto const bogus_activation_token = "foobar";
                auto const bogus_time = 0;

                auto const modifier_intersection = key_event->modifiers() & trigger.value().modifiers;
                if (modifier_intersection != key_event->modifiers() ||
                    modifier_intersection != trigger.value().modifiers)
                {
                    if (began)
                    {
                        action->send_end_event(bogus_time, bogus_activation_token);
                        began = false;
                    }

                    return false;
                }

                // I'm sure this is not going to blow up in my face later :)
                if (static_cast<uint32_t>(key_event->keysym()) != trigger.value().keysym)
                {
                    if (began)
                    {
                        action->send_end_event(bogus_time, bogus_activation_token);
                        began = false;
                    }

                    return false;
                }

                action->send_begin_event(bogus_time, bogus_activation_token);
                began = true;
            }

            // Invalid token
            return false;
        }
    };

    std::shared_ptr<mi::CompositeEventFilter> const cef;
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

void InputTriggerRegistrationManagerV1::Instance::register_modifier_tap_trigger(uint32_t, uint32_t, struct wl_resource*)
{
}

void InputTriggerRegistrationManagerV1::Instance::register_modifier_hold_trigger(uint32_t, struct wl_resource*)
{
}

void InputTriggerRegistrationManagerV1::Instance::register_pointer_trigger(uint32_t, struct wl_resource*)
{
}

void InputTriggerRegistrationManagerV1::Instance::register_touch_drag_trigger(uint32_t, uint32_t, struct wl_resource*)
{
}

void InputTriggerRegistrationManagerV1::Instance::register_touch_tap_trigger(uint32_t, uint32_t, struct wl_resource*)
{
}

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    new ActionControl{cef, id};
}
}
}

auto mf::create_input_trigger_registration_manager_v1(
    wl_display* display, std::shared_ptr<mi::CompositeEventFilter> const& cef)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<mf::InputTriggerRegistrationManagerV1>(display, cef);
}
