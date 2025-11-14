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
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/input/xkb_mapper.h"
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
        if (auto const keyboard_trigger = static_cast<KeyboardSymTrigger*>(wayland::InputTriggerV1::from(trigger)))
        {
            auto const token = "foo";
            trigger_filter = std::make_shared<KeyboardEventFilter>(wayland::make_weak(keyboard_trigger), token);
            keyboard_trigger->extra_data.action_token = token;

            cef->prepend(trigger_filter);
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

        static bool protocol_and_event_modifiers_match(uint32_t protocol_modifiers, MirInputEventModifiers event_mods)
        {
            using ProtocolModifiers = wayland::InputTriggerRegistrationManagerV1::Modifiers;
            namespace mi = mir::input;

            // Canonicalise event modifiers to match how Mir represents them elsewhere.
            event_mods = mi::expand_modifiers(event_mods);

            MirInputEventModifiers required = 0;
            MirInputEventModifiers allowed = 0;

            // Pure function: compute per-kind (required, allowed) masks and return them.
            auto handle_kind =
                [&](uint32_t protocol_generic,
                    uint32_t protocol_left,
                    uint32_t protocol_right,
                    MirInputEventModifiers mir_generic,
                    MirInputEventModifiers mir_left,
                    MirInputEventModifiers mir_right) -> std::pair<MirInputEventModifiers, MirInputEventModifiers>
            {
                MirInputEventModifiers req = 0;
                MirInputEventModifiers allow = 0;

                // Did the client specify a generic modifier? or a specific side?
                bool p_generic = (protocol_modifiers & protocol_generic) != 0;
                bool p_left = (protocol_left != 0 && (protocol_modifiers & protocol_left) != 0);
                bool p_right = (protocol_right != 0 && (protocol_modifiers & protocol_right) != 0);

                // Client explicitly requested side(s): require those sides and the generic bit.
                if (p_left || p_right)
                {
                    if (p_left)
                        req |= mir_left;
                    if (p_right)
                        req |= mir_right;
                    req |= mir_generic;

                    // Allow generic and both side bits.
                    allow |= mir_generic | mir_left | mir_right;
                }
                else if (p_generic)
                {
                    // Client requested generic only -> require generic, but allow either side.
                    req |= mir_generic;
                    allow |= mir_generic | mir_left | mir_right;
                }
                // else: client didn't request this kind -> leave both masks zero (disallow).

                return {req, allow};
            };

            struct Kind
            {
                uint32_t protocol_generic;
                uint32_t protocol_left;
                uint32_t protocol_right;
                MirInputEventModifiers mir_generic;
                MirInputEventModifiers mir_left;
                MirInputEventModifiers mir_right;
            };

            constexpr Kind kinds[] = {
                // ctrl
                {ProtocolModifiers::ctrl,
                 ProtocolModifiers::ctrl_left,
                 ProtocolModifiers::ctrl_right,
                 mir_input_event_modifier_ctrl,
                 mir_input_event_modifier_ctrl_left,
                 mir_input_event_modifier_ctrl_right},

                // alt
                {ProtocolModifiers::alt,
                 ProtocolModifiers::alt_left,
                 ProtocolModifiers::alt_right,
                 mir_input_event_modifier_alt,
                 mir_input_event_modifier_alt_left,
                 mir_input_event_modifier_alt_right},

                // shift
                {ProtocolModifiers::shift,
                 ProtocolModifiers::shift_left,
                 ProtocolModifiers::shift_right,
                 mir_input_event_modifier_shift,
                 mir_input_event_modifier_shift_left,
                 mir_input_event_modifier_shift_right},

                // meta
                {ProtocolModifiers::meta,
                 ProtocolModifiers::meta_left,
                 ProtocolModifiers::meta_right,
                 mir_input_event_modifier_meta,
                 mir_input_event_modifier_meta_left,
                 mir_input_event_modifier_meta_right},

                // sym (protocol-generic only; left/right fields = 0)
                {ProtocolModifiers::sym, 0, 0, mir_input_event_modifier_sym, 0, 0},

                // function (protocol-generic only)
                {ProtocolModifiers::function, 0, 0, mir_input_event_modifier_function, 0, 0},
            };

            // Given the client specified modifier, construct a mask containing the required bits (if a side is
            // defined), and a mask containing the allowed bits (if a side is not defined)
            for (auto const& k : kinds)
            {
                auto p = handle_kind(
                    k.protocol_generic, k.protocol_left, k.protocol_right, k.mir_generic, k.mir_left, k.mir_right);
                required |= p.first;
                allowed |= p.second;
            }

            // Required bits must be present
            if ((event_mods & required) != required)
                return false;

            // No bits are allowed outside 'allowed'
            if ((event_mods & ~allowed) != 0)
                return false;

            return true;
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

                auto const event_mods = mi::expand_modifiers(key_event->modifiers());
                auto const trigger_mods = mi::expand_modifiers(trigger.value().modifiers);

                // FIXME if the modifiers include shift, and the key is
                // alphabetic key, we receive XKB_KEY_<uppercase key>. If the
                // client requests a lowercase keysym, then it will never be
                // triggered.
                if (!protocol_and_event_modifiers_match(trigger_mods, event_mods) ||
                    static_cast<uint32_t>(key_event->keysym()) != trigger.value().keysym)
                {
                    if (began)
                        action.value().send_end_event(bogus_time, bogus_activation_token);

                    began = false;
                    return false;
                }

                if (key_event->action() == mir_keyboard_action_down)
                {
                    action.value().send_begin_event(bogus_time, bogus_activation_token);
                    began = true;
                    return true;
                }

                if (key_event->action() == mir_keyboard_action_up && began)
                {
                    action.value().send_end_event(bogus_time, bogus_activation_token);
                    began = false;
                    return true;
                }

                return false;
            }

            // Invalid token
            return false;
        }
    };

    std::shared_ptr<mi::CompositeEventFilter> const cef;
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
    auto action_control = new ActionControl{cef, id};
    action_control->send_done_event("foo");
}
}
}

auto mf::create_input_trigger_registration_manager_v1(
    wl_display* display, std::shared_ptr<mi::CompositeEventFilter> const& cef)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<mf::InputTriggerRegistrationManagerV1>(display, cef);
}
