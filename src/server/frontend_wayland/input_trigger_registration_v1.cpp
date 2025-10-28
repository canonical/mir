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
#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/wayland/weak.h"

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
        modifiers{modifiers},
        keysym{keysym}
    {
    }

    uint32_t const modifiers;
    uint32_t const keysym;
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
            auto const filter = std::make_shared<KeyboardEventFilter>(wayland::make_weak(keyboard_trigger));
            triggers_to_filters.insert({trigger, filter});
            cef->prepend(filter);

            // keyboard_trigger->send_done_event();
            // TODO: Set up token -> action control / trigger association
            send_done_event("foo");
        }
    }
    virtual void drop_input_trigger_event(struct wl_resource* /*trigger*/) override
    {
    }

private:
    struct KeyboardEventFilter : public mi::EventFilter
    {
        wayland::Weak<KeyboardSymTrigger> const trigger;

        explicit KeyboardEventFilter(wayland::Weak<KeyboardSymTrigger> trigger) :
            trigger{std::move(trigger)}
        {
        }

        bool handle(MirEvent const& event) override
        {
            if(event.type() != mir_event_type_input)
                return false;

            auto const* input_event = event.to_input();
            if(input_event->input_type() != mir_input_event_type_key)
                return false;

            auto const* key_event = input_event->to_keyboard();

            auto const modifier_intersection = key_event->modifiers() & trigger.value().modifiers;
            if (modifier_intersection != key_event->modifiers() || modifier_intersection != trigger.value().modifiers)
                return false;

            // I'm sure this is not going to blow up in my face later :)
            if(static_cast<uint32_t>(key_event->keysym()) != trigger.value().keysym)
                return false;

            // TODO: Get the action from the trigger and send a begin event

            return true;
        }
    };

    std::shared_ptr<mi::CompositeEventFilter> const cef;
    std::unordered_map<wl_resource*, std::shared_ptr<mi::EventFilter>> triggers_to_filters;
};

// The trigger is registered with the composite event filter when its added to a control object
void InputTriggerRegistrationManagerV1::Instance::register_keyboard_sym_trigger(
    uint32_t modifiers, uint32_t keysym, struct wl_resource* id)
{
    new KeyboardSymTrigger{modifiers, keysym, id};
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
