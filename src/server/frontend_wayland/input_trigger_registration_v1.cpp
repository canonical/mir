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
#include "ext-input-trigger-action-v1_wrapper.h"
#include "input_trigger_action_v1.h"
#include "input_trigger_data.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/executor.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/event_filter.h>
#include <mir/input/xkb_mapper.h>
#include <mir/log.h>
#include <mir/shell/token_authority.h>
#include <mir/synchronised.h>
#include <mir/wayland/weak.h>
#include <mir_toolkit/events/enums.h>

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
        std::shared_ptr<mir::Synchronised<InputTriggerData>> const& itd,
        std::shared_ptr<msh::TokenAuthority> const& ta,
        std::shared_ptr<mi::CompositeEventFilter> const& cef) :
        Global{display, Version<1>{}},
        itd{itd},
        ta{ta},
        cef{cef}
    {
    }

    class Instance : public wayland::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<mir::Synchronised<InputTriggerData>> const& itd,
            std::shared_ptr<msh::TokenAuthority> const& token_authority,
            std::shared_ptr<input::CompositeEventFilter> const& cef) :
            wayland::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            itd{itd},
            ta{token_authority},
            cef{cef}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        auto has_trigger(wayland::InputTriggerV1 const*) const -> bool;

        std::shared_ptr<mir::Synchronised<InputTriggerData>> const itd;
        std::shared_ptr<msh::TokenAuthority> const ta;
        std::shared_ptr<input::CompositeEventFilter> const cef;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, itd, ta, cef};
    }

private:
    std::shared_ptr<mir::Synchronised<InputTriggerData>> const itd;
    std::shared_ptr<msh::TokenAuthority> const ta;
    std::shared_ptr<input::CompositeEventFilter> const cef;
};

KeyboardEventFilter::KeyboardEventFilter(
    wayland::Weak<wayland::InputTriggerActionV1 const> const& action,
    wayland::Weak<frontend::KeyboardSymTrigger const> const& trigger,
    std::shared_ptr<shell::TokenAuthority> const& token_authority,
    std::shared_ptr<KeyboardStateTracker> const& keyboard_state) :
    action{action},
    trigger{trigger},
    token_authority{token_authority},
    keyboard_state{keyboard_state}
{
}

void KeyboardStateTracker::on_key_down(uint32_t keysym)
{
    pressed_keysyms.insert(keysym);
}

void KeyboardStateTracker::on_key_up(uint32_t keysym)
{
    pressed_keysyms.erase(keysym);
}

auto KeyboardStateTracker::keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool
{
    if (!case_insensitive)
        return pressed_keysyms.find(keysym) != pressed_keysyms.end();

    // Only perform case mapping for ASCII letters. For other keysyms, fall back to exact match.
    uint32_t lower = keysym;
    uint32_t upper = keysym;

    if (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z)
    {
        lower = keysym + (XKB_KEY_a - XKB_KEY_A);
    }
    else if (keysym >= XKB_KEY_a && keysym <= XKB_KEY_z)
    {
        upper = keysym - (XKB_KEY_a - XKB_KEY_A);
    }

    return pressed_keysyms.find(lower) != pressed_keysyms.end() || pressed_keysyms.find(upper) != pressed_keysyms.end();
}

bool KeyboardStateTracker::protocol_and_event_modifiers_match(
    uint32_t protocol_modifiers, MirInputEventModifiers event_mods)
{
    using ProtocolModifiers = wayland::InputTriggerRegistrationManagerV1::Modifiers;

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
        auto p =
            handle_kind(k.protocol_generic, k.protocol_left, k.protocol_right, k.mir_generic, k.mir_left, k.mir_right);
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

bool KeyboardEventFilter::handle(MirEvent const& event)
{
    if (!trigger)
        return false;

    if (event.type() != mir_event_type_input)
        return false;

    auto const* input_event = event.to_input();
    if (input_event->input_type() != mir_input_event_type_key)
        return false;

    auto const* key_event = input_event->to_keyboard();

    if (key_event->action() == mir_keyboard_action_down)
        keyboard_state->on_key_down(key_event->keysym());
    else if (key_event->action() == mir_keyboard_action_up)
        keyboard_state->on_key_up(key_event->keysym());

    auto const event_mods = mi::expand_modifiers(key_event->modifiers());
    auto const trigger_mods = mi::expand_modifiers(trigger.value().modifiers);
    auto const modifiers_match = KeyboardStateTracker::protocol_and_event_modifiers_match(trigger_mods, event_mods);

    auto const trigger_mods_contain_shift =
        ((trigger_mods & mir_input_event_modifier_shift) | (trigger_mods & mir_input_event_modifier_shift_left) |
         (trigger_mods & mir_input_event_modifier_shift_right)) != 0;

    auto const keysym_matches =
        keyboard_state->keysym_is_pressed(trigger.value().keysym, trigger_mods_contain_shift);

    if (!modifiers_match || !keysym_matches)
    {
        if (began)
        {
            auto const activation_token = std::string{token_authority->issue_token(std::nullopt)};
            auto const timestamp = mir_input_event_get_wayland_timestamp(mir_keyboard_event_input_event(key_event));
            action.value().send_end_event(timestamp, activation_token);
            began = false;
        }

        return false;
    }

    // If the trigger keysym is pressed (either just pressed or was already pressed),
    // ensure we send a begin event if we haven't already.
    if (!began)
    {
        auto const activation_token = std::string{token_authority->issue_token(std::nullopt)};
        auto const timestamp = mir_input_event_get_wayland_timestamp(mir_keyboard_event_input_event(key_event));
        action.value().send_begin_event(timestamp, activation_token);
        began = true;
        return true;
    }

    return false;
}

auto KeyboardEventFilter::is_same_trigger(wayland::InputTriggerV1 const* other) const -> bool
{
    if (auto const other_keyboard_trigger = KeyboardSymTrigger::from(other))
    {
        return trigger && trigger.value().keysym == other_keyboard_trigger->keysym &&
               trigger.value().modifiers == other_keyboard_trigger->modifiers;
    }
    return false;
}

ActionControl::ActionControl(
    std::shared_ptr<mir::Synchronised<InputTriggerData>> const& itd,
    std::shared_ptr<msh::TokenAuthority> const& ta,
    std::shared_ptr<mi::CompositeEventFilter> const& cef,
    std::string_view token,
    struct wl_resource* id) :
    mir::wayland::InputTriggerActionControlV1{id, Version<1>{}},
    itd{itd},
    ta{ta},
    cef{cef},
    token{token}
{
}

void ActionControl::add_trigger_pending(wayland::InputTriggerV1 const* trigger)
{
    pending_triggers.insert(trigger);
}

void ActionControl::add_trigger_immediate(wayland::InputTriggerV1 const* trigger)
{
    action.value().add_trigger(trigger);
}

void ActionControl::drop_trigger_pending(wayland::InputTriggerV1 const* trigger)
{
    pending_triggers.erase(trigger);
}

void ActionControl::drop_trigger_immediate(wayland::InputTriggerV1 const* trigger)
{
    action.value().drop_trigger(trigger);
}

void ActionControl::add_input_trigger_event(struct wl_resource* trigger)
{
    if(auto const* input_trigger = wayland::InputTriggerV1::from(trigger))
    {
        if (!action)
            add_trigger_pending(input_trigger);
        else
            add_trigger_immediate(input_trigger);
    }
}

void ActionControl::drop_input_trigger_event(struct wl_resource* trigger)
{
    if(auto const* input_trigger = wayland::InputTriggerV1::from(trigger))
    {
        if (!action)
            drop_trigger_pending(input_trigger);
        else
            drop_trigger_immediate(input_trigger);
    }
}

void ActionControl::install_action(wayland::Weak<frontend::InputTriggerActionV1> action)
{
    this->action = action;

    // Add the pending triggers to the composite event filter
    for (auto const* trigger : pending_triggers)
        add_trigger_immediate(trigger);

    // Don't need this anymore
    pending_triggers.clear();
}

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
        mir::log_debug("%s already registered", keyboard_trigger->to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_code_trigger(
    uint32_t, uint32_t, struct wl_resource*)
{
}

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    auto const token = ta->issue_token(
        [itd = itd](auto const& token)
        {
            // If the token becomes invalid before an action is associated with
            // it, we should clean up the action control object.
            // When the client tries obtaining the action object using the token,
            // they will get an `unavailable` event.
            auto const itd_ = itd->lock();
            auto const& actions = itd_->actions;
            auto& action_controls = itd_->action_controls;
            auto& revoked_tokens = itd_->revoked_tokens;

            auto const token_string = std::string{token};
            if(!actions.contains(token_string))
                action_controls.erase(token_string);

            revoked_tokens.enqueue(token_string);
        });

    auto const token_string = static_cast<std::string>(token);
    auto const action_control = new ActionControl{itd, ta, cef, token_string, id};

    itd->lock()->action_controls.insert({token_string, wayland::make_weak(action_control)});

    // Tell the client that the action has been successfully registered.
    // They then should call
    // `InputTriggerActionManagerV1::get_input_trigger_action` using the
    // token we supply here.
    action_control->send_done_event(token_string);
}

auto InputTriggerRegistrationManagerV1::Instance::has_trigger(wayland::InputTriggerV1 const* trigger) const -> bool
{
    auto const itd_ = itd->lock();
    auto const& actions = itd_->actions;

    return std::ranges::any_of(
        actions, [trigger](auto const pair) { return pair.second.value().has_trigger(trigger); });
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

auto create_input_trigger_registration_manager_v1(
    wl_display* display,
    std::shared_ptr<mir::Synchronised<InputTriggerData>> const& itd,
    std::shared_ptr<msh::TokenAuthority> const& ta,
    std::shared_ptr<mi::CompositeEventFilter> const& cef)
    -> std::shared_ptr<wayland::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<mf::InputTriggerRegistrationManagerV1>(display, itd, ta, cef);
}
}
}
