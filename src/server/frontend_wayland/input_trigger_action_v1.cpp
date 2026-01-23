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

#include "input_trigger_action_v1.h"
#include "input_trigger_data.h"
#include "input_trigger_registration_v1.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/executor.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/event_filter.h>
#include <mir/input/xkb_mapper.h>
#include <mir/shell/token_authority.h>
#include <mir/time/alarm.h>
#include <mir/time/alarm_factory.h>
#include <mir/time/clock.h>
#include <mir/wayland/weak.h>

#include <atomic>
#include <unordered_set>

namespace mi = mir::input;

namespace mir
{
namespace frontend
{

struct FilterContext
{
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<time::AlarmFactory> const alarm_factory;
    std::shared_ptr<time::Clock> const clock;
    std::shared_ptr<shell::TokenAuthority> const token_authority;
};

struct ManagerContext
{
    std::shared_ptr<InputTriggerData> const itd;
    std::shared_ptr<mi::CompositeEventFilter> const cef;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<time::AlarmFactory> const alarm_factory;
    std::shared_ptr<time::Clock> const clock;
    std::shared_ptr<shell::TokenAuthority> const token_authority;
};

class InputTriggerActionManagerV1 : public wayland::InputTriggerActionManagerV1::Global
{
public:
    InputTriggerActionManagerV1(wl_display* display, ManagerContext const& context) :
        Global{display, Version<1>{}},
        context{context}
    {
    }

private:
    struct KeyboardEventFilter : public mi::EventFilter
    {
        wayland::Weak<KeyboardSymTrigger> const trigger;
        wayland::Weak<wayland::InputTriggerActionV1> const action;
        FilterContext const context;

        std::atomic<bool> began{false};
        // This extends slightly before `began`. It is set when the key combo
        // is first completed, instead of being set after the hold delay.
        std::unique_ptr<time::Alarm> hold_alarm;

        // TODO key state tracking should be moved into its own class, to be shared with all active filters
        std::unordered_set<uint32_t> pressed_keysyms;

        explicit KeyboardEventFilter(
            wayland::InputTriggerActionV1* action,
            wayland::Weak<KeyboardSymTrigger> trigger,
            FilterContext const& context) :
            trigger{std::move(trigger)},
            action{wayland::make_weak(action)},
            context{context}
        {
        }

        static bool protocol_and_event_modifiers_match(uint32_t protocol_modifiers, MirInputEventModifiers event_mods)
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

            if (key_event->action() == mir_keyboard_action_down)
                pressed_keysyms.insert(key_event->keysym());
            else if (key_event->action() == mir_keyboard_action_up)
                pressed_keysyms.erase(key_event->keysym());

            auto const event_mods = mi::expand_modifiers(key_event->modifiers());
            auto const trigger_mods = mi::expand_modifiers(trigger.value().modifiers);
            auto const modifiers_match = protocol_and_event_modifiers_match(trigger_mods, event_mods);

            auto const trigger_mods_contain_shift = ((trigger_mods & mir_input_event_modifier_shift) |
                                                     (trigger_mods & mir_input_event_modifier_shift_left) |
                                                     (trigger_mods & mir_input_event_modifier_shift_right)) != 0;

            auto const keysym_matches =
                keysym_exists_in_set(trigger.value().keysym, pressed_keysyms, trigger_mods_contain_shift);

            if (trigger)
            {
                if (!modifiers_match || !keysym_matches)
                {
                    if (began)
                    {
                        auto const activation_token = std::string{context.token_authority->issue_token(std::nullopt)};
                        auto const timestamp =
                            mir_input_event_get_wayland_timestamp(mir_keyboard_event_input_event(key_event));
                        action.value().send_end_event(timestamp, activation_token);
                        began = false;
                    }

                    return false;
                }

                // If the trigger keysym is pressed (either just pressed or was already pressed),
                // ensure we send a begin event if we haven't already.
                if (!began)
                {
                    auto const activation_token = std::string{context.token_authority->issue_token(std::nullopt)};
                    auto const timestamp =
                        mir_input_event_get_wayland_timestamp(mir_keyboard_event_input_event(key_event));
                    action.value().send_begin_event(timestamp, activation_token);
                    began = true;
                    return true;
                }

                return false;
            }

            return false;
        }

        static auto keysym_exists_in_set(
            uint32_t keysym, std::unordered_set<uint32_t> const& pressed, bool case_insensitive) -> bool
        {
            if (!case_insensitive)
                return pressed.find(keysym) != pressed.end();

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

            return pressed.find(lower) != pressed.end() || pressed.find(upper) != pressed.end();
        }
    };

    class Instance : public wayland::InputTriggerActionManagerV1
    {
        ManagerContext const& context;
        std::vector<std::shared_ptr<mi::EventFilter>> trigger_filters;

        void get_input_trigger_action(std::string const& token, struct wl_resource* id) override
        {
            auto const pa = context.itd->pending_actions.lock();
            if (auto iter = pa->find(token); iter != pa->cend())
            {
                // TODO validate token
                pa->erase(iter);

                auto const action = new InputTriggerActionV1(id);
                auto const trigger = iter->second;
                if (auto const keyboard_trigger = static_cast<KeyboardSymTrigger*>(&trigger.value()))
                {
                    auto const filter = std::make_shared<KeyboardEventFilter>(
                        action,
                        wayland::make_weak(keyboard_trigger),
                        FilterContext{
                            context.wayland_executor, context.alarm_factory, context.clock, context.token_authority});
                    trigger_filters.emplace_back();
                    context.cef->prepend(filter);

                    auto const remove_filter = [this, filter]()
                    {
                        trigger_filters.erase(
                            std::remove(trigger_filters.begin(), trigger_filters.end(), filter),
                            trigger_filters.end());
                    };

                    action->add_destroy_listener(remove_filter);
                    trigger.value().add_destroy_listener(remove_filter);
                }
            }
            else
            {
                auto const action = new InputTriggerActionV1(id);
                action->send_unavailable_event();
            }
        }

    public:
        Instance(wl_resource* new_ext_input_trigger_action_manager_v1, ManagerContext const& context) :
            InputTriggerActionManagerV1{new_ext_input_trigger_action_manager_v1, Version<1>{}},
            context{context}
        {
        }
    };

    void bind(wl_resource* new_ext_input_trigger_action_manager_v1) override
    {
        new Instance{new_ext_input_trigger_action_manager_v1, context};
    }

    ManagerContext const context;
};

auto create_input_trigger_action_manager_v1(
    wl_display* display,
    std::shared_ptr<InputTriggerData> const& itd,
    std::shared_ptr<mi::CompositeEventFilter> const& cef,
    std::shared_ptr<mir::Executor> const& wayland_executor,
    std::shared_ptr<time::AlarmFactory> const& alarm_factory,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<shell::TokenAuthority> const& token_authority)
    -> std::shared_ptr<wayland::InputTriggerActionManagerV1::Global>
{
    return std::make_shared<InputTriggerActionManagerV1>(
        display, ManagerContext{itd, cef, wayland_executor, alarm_factory, clock, token_authority});
}
}
}
