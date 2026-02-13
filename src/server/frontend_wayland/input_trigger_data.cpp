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

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/input/xkb_mapper.h>
#include <mir_toolkit/events/enums.h>
#include <mir/log.h>

#include <wayland-server-core.h>

#include <algorithm>
#include <string>
#include <vector>

namespace mf = mir::frontend;
namespace mi = mir::input;
namespace msh = mir::shell;
namespace mw = mir::wayland;

auto mf::InputTriggerModifiers::to_string() const -> std::string
{
    if (value == mir_input_event_modifier_none)
        return "none";

    std::vector<std::string> parts;

    static auto constexpr pairs = {
        std::pair{mir_input_event_modifier_shift_left, "shift_left"},
        std::pair{mir_input_event_modifier_shift_right, "shift_right"},
        std::pair{mir_input_event_modifier_shift, "shift"},

        std::pair{mir_input_event_modifier_ctrl_left, "ctrl_left"},
        std::pair{mir_input_event_modifier_ctrl_right, "ctrl_right"},
        std::pair{mir_input_event_modifier_ctrl, "ctrl"},

        std::pair{mir_input_event_modifier_alt_left, "alt_left"},
        std::pair{mir_input_event_modifier_alt_right, "alt_right"},
        std::pair{mir_input_event_modifier_alt, "alt"},

        std::pair{mir_input_event_modifier_meta_left, "meta_left"},
        std::pair{mir_input_event_modifier_meta_right, "meta_right"},
        std::pair{mir_input_event_modifier_meta, "meta"},

        std::pair{mir_input_event_modifier_sym, "sym"},
        std::pair{mir_input_event_modifier_function, "function"},
        std::pair{mir_input_event_modifier_caps_lock, "caps_lock"},
        std::pair{mir_input_event_modifier_num_lock, "num_lock"},
        std::pair{mir_input_event_modifier_scroll_lock, "scroll_lock"},
    };

    for (auto const& [modifier_bit, modifier_name] : pairs)
    {
        if (value & modifier_bit)
            parts.push_back(modifier_name);
    }

    if (parts.empty())
        return "unknown";

    std::string result;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
            result += " | ";
        result += parts[i];
    }
    return result;
}

auto mf::InputTriggerModifiers::from_protocol(uint32_t protocol_mods) -> InputTriggerModifiers
{
    return from_protocol(protocol_mods, false);
}

auto mf::InputTriggerModifiers::from_protocol(uint32_t protocol_mods, bool shift_adjustment) -> InputTriggerModifiers
{
    using PM = mw::InputTriggerRegistrationManagerV1::Modifiers;

    if (protocol_mods == 0)
        return InputTriggerModifiers{mir_input_event_modifier_none};

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

    MirInputEventModifiers result = mir_input_event_modifier_none;
    for (auto const& [protocol_modifier, mir_modifier] : mappings)
    {
        if (protocol_mods & protocol_modifier)
            result |= mir_modifier;
    }

    if (shift_adjustment)
        result |= mir_input_event_modifier_shift;

    return InputTriggerModifiers{result};
}

void mf::KeyboardStateTracker::on_key_down(uint32_t keysym, uint32_t scancode)
{
    pressed_keysyms.insert(keysym);
    pressed_scancodes.insert(scancode);
}

void mf::KeyboardStateTracker::on_key_up(uint32_t keysym, uint32_t scancode)
{
    pressed_keysyms.erase(keysym);
    pressed_scancodes.erase(scancode);
}

auto mf::KeyboardStateTracker::keysym_is_pressed(uint32_t keysym, bool case_insensitive) const -> bool
{
    if (!case_insensitive)
        return pressed_keysyms.contains(keysym);

    // Only perform case mapping only for a-z and A-Z
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

    return pressed_keysyms.contains(lower) || pressed_keysyms.contains(upper);
}

auto mf::KeyboardStateTracker::scancode_is_pressed(uint32_t scancode) const -> bool
{
    return pressed_scancodes.contains(scancode);
}


mf::KeyboardTrigger::KeyboardTrigger(InputTriggerModifiers modifiers, struct wl_resource* id) :
    InputTriggerV1{id, Version<1>{}},
    modifiers{modifiers}
{
}

bool mf::KeyboardTrigger::modifiers_match(mf::InputTriggerModifiers modifiers, mf::InputTriggerModifiers event_mods)
{
    // Special case, if the event comes explicitly with no modifiers, it should
    // only match if the protocol also specified no modifiers.
    if (event_mods.raw_value() == MirInputEventModifier::mir_input_event_modifier_none)
        return modifiers.raw_value() == MirInputEventModifier::mir_input_event_modifier_none;

    MirInputEventModifiers required = 0;
    MirInputEventModifiers allowed = 0;

    // Pure function: compute per-kind (required, allowed) masks and return them.
    auto const protocol_value = modifiers.raw_value();
    auto handle_kind =
        [&](MirInputEventModifiers mir_generic,
            MirInputEventModifiers mir_left,
            MirInputEventModifiers mir_right) -> std::pair<MirInputEventModifiers, MirInputEventModifiers>
    {
        MirInputEventModifiers req = 0;
        MirInputEventModifiers allow = 0;

        // Did the client specify a generic modifier? or a specific side?
        bool p_generic = (protocol_value & mir_generic) != 0;
        bool p_left = (mir_left != 0 && (protocol_value & mir_left) != 0);
        bool p_right = (mir_right != 0 && (protocol_value & mir_right) != 0);

        // Client explicitly requested side(s): require those sides and the generic bit.
        if (p_left || p_right)
        {
            req |= mir_generic;
            allow |= mir_generic;

            if (p_left)
            {
                req |= mir_left;
                allow |= mir_left;
            }
            if (p_right)
            {
                req |= mir_right;
                allow |= mir_right;
            }
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
        MirInputEventModifiers mir_generic;
        MirInputEventModifiers mir_left;
        MirInputEventModifiers mir_right;
    };

    constexpr Kind kinds[] = {
        // ctrl
        {
            mir_input_event_modifier_ctrl,
            mir_input_event_modifier_ctrl_left,
            mir_input_event_modifier_ctrl_right,
        },

        // alt
        {
            mir_input_event_modifier_alt,
            mir_input_event_modifier_alt_left,
            mir_input_event_modifier_alt_right,
        },

        // shift
        {
            mir_input_event_modifier_shift,
            mir_input_event_modifier_shift_left,
            mir_input_event_modifier_shift_right,
        },

        // meta
        {
            mir_input_event_modifier_meta,
            mir_input_event_modifier_meta_left,
            mir_input_event_modifier_meta_right,
        },

        // sym (protocol-generic only; left/right fields = 0)
        {mir_input_event_modifier_sym, 0, 0},

        // function (protocol-generic only)
        {mir_input_event_modifier_function, 0, 0},
    };

    // Given the client specified modifier, construct a mask containing the required bits (if a side is
    // defined), and a mask containing the allowed bits (if a side is not defined)
    for (auto const& k : kinds)
    {
        auto p = handle_kind(k.mir_generic, k.mir_left, k.mir_right);
        required |= p.first;
        allowed |= p.second;
    }

    // Required bits must be present
    if ((event_mods.raw_value() & required) != required)
        return false;

    // No bits are allowed outside 'allowed'
    if ((event_mods.raw_value() & ~allowed) != 0)
        return false;

    return true;
}

mf::KeyboardSymTrigger::KeyboardSymTrigger(
    mf::InputTriggerModifiers modifiers, uint32_t keysym, struct wl_resource* id) :
    KeyboardTrigger{modifiers, id},
    keysym{keysym}
{
}

auto mf::KeyboardSymTrigger::from(mw::InputTriggerV1* trigger) -> KeyboardSymTrigger*
{
    return static_cast<KeyboardSymTrigger*>(trigger);
}

auto mf::KeyboardSymTrigger::from(mw::InputTriggerV1 const* trigger) -> KeyboardSymTrigger const*
{
    return static_cast<KeyboardSymTrigger const*>(trigger);
}

auto mf::KeyboardSymTrigger::to_c_str() const -> char const*
{
    static char buf[256];

    auto const end = std::format_to(
        buf,
        "KeyboardSymTrigger{{client={}, keysym={}, modifiers={}}}",
        static_cast<void*>(wl_resource_get_client(resource)),
        keysym,
        modifiers.to_string());
    *end = '\0';

    return buf;
}

auto mf::KeyboardSymTrigger::matches(
    MirEvent const& ev, std::shared_ptr<KeyboardStateTracker> const& keyboard_state) const -> bool
{
    auto const event_modifiers = InputTriggerModifiers{ev.to_input()->to_keyboard()->modifiers()};
    if (!KeyboardTrigger::modifiers_match(modifiers, event_modifiers))
        return false;

    auto const mods_value = modifiers.raw_value();
    auto const trigger_mods_contain_shift =
        ((mods_value & mir_input_event_modifier_shift) | (mods_value & mir_input_event_modifier_shift_left) |
         (mods_value & mir_input_event_modifier_shift_right)) != 0;

    return keyboard_state->keysym_is_pressed(keysym, trigger_mods_contain_shift);
}

bool mir::frontend::KeyboardSymTrigger::is_same_trigger(wayland::InputTriggerV1 const* other) const
{
    if (auto const other_sym = KeyboardSymTrigger::from(other))
    {
        return keysym == other_sym->keysym && modifiers == other_sym->modifiers;
    }

    return false;
}

mf::KeyboardCodeTrigger::KeyboardCodeTrigger(
    InputTriggerModifiers modifiers, uint32_t scancode, struct wl_resource* id) :
    KeyboardTrigger{modifiers, id},
    scancode{scancode}
{
}

auto mf::KeyboardCodeTrigger::from(mw::InputTriggerV1* trigger) -> KeyboardCodeTrigger*
{
    return static_cast<KeyboardCodeTrigger*>(trigger);
}

auto mf::KeyboardCodeTrigger::from(mw::InputTriggerV1 const* trigger) -> KeyboardCodeTrigger const*
{
    return static_cast<KeyboardCodeTrigger const*>(trigger);
}

auto mf::KeyboardCodeTrigger::to_c_str() const -> char const*
{
    static char buf[256];

    auto const end = std::format_to(
        buf,
        "KeyboardCodeTrigger{{client={}, scancode={}, modifiers={}}}",
        static_cast<void*>(wl_resource_get_client(resource)),
        scancode,
        modifiers.to_string());
    *end = '\0';

    return buf;
}

auto mf::KeyboardCodeTrigger::matches(
    MirEvent const& ev, std::shared_ptr<KeyboardStateTracker> const& keyboard_state) const -> bool
{
    auto const event_modifiers = InputTriggerModifiers{ev.to_input()->to_keyboard()->modifiers()};
    if (!KeyboardTrigger::modifiers_match(modifiers, event_modifiers))
        return false;

    return keyboard_state->scancode_is_pressed(scancode);
}

auto mf::KeyboardCodeTrigger::is_same_trigger(wayland::InputTriggerV1 const* other) const -> bool
{
    if (auto const other_code = KeyboardCodeTrigger::from(other))
    {
        return scancode == other_code->scancode && modifiers == other_code->modifiers;
    }

    return false;
}
mf::InputTriggerActionV1::InputTriggerActionV1(
    std::shared_ptr<msh::TokenAuthority> const& token_authority,
    std::shared_ptr<KeyboardStateTracker> const& keyboard_state,
    wl_resource* id) :
    mw::InputTriggerActionV1{id, Version<1>{}},
    token_authority{token_authority},
    keyboard_state{keyboard_state}
{
}

auto mf::InputTriggerActionV1::has_trigger(mw::InputTriggerV1 const* trigger) const -> bool
{
    return std::ranges::any_of(
        triggers, [trigger](auto const& action_trigger) { return action_trigger.value().is_same_trigger(trigger); });
}

void mf::InputTriggerActionV1::add_trigger(mw::InputTriggerV1 const* trigger)
{
    if (auto const* input_trigger = dynamic_cast<frontend::InputTriggerV1 const*>(trigger))
    {
        triggers.push_back(wayland::make_weak(input_trigger));
        return;
    }

    mir::log_error("Invalid trigger provided");
}

void mf::InputTriggerActionV1::drop_trigger(mw::InputTriggerV1 const* trigger)
{
    std::erase_if(
        triggers, [&](auto const& action_trigger) { return action_trigger.value().is_same_trigger(trigger); });
}

bool mf::InputTriggerActionV1::matches(MirEvent const& event)
{
    auto const matched = std::ranges::any_of(
        triggers, [&](auto const& action_trigger) { return action_trigger.value().matches(event, keyboard_state); });

    if (!matched)
    {
        if (began)
        {
            auto const activation_token = std::string{token_authority->issue_token(std::nullopt)};
            auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());
            send_end_event(timestamp, activation_token);
            began = false;
        }

        return false;
    }

    if (!began)
    {
        auto const activation_token = std::string{token_authority->issue_token(std::nullopt)};
        auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());
        send_begin_event(timestamp, activation_token);
        began = true;
        return true;
    }

    return false;
}

mf::ActionControl::ActionControl(std::string_view token, struct wl_resource* id) :
    mw::InputTriggerActionControlV1{id, Version<1>{}},
    token{token}
{
}

void mf::ActionControl::add_trigger_pending(mw::InputTriggerV1 const* trigger)
{
    pending_triggers.insert(trigger);
}

void mf::ActionControl::add_trigger_immediate(mw::InputTriggerV1 const* trigger)
{
    action.value().add_trigger(trigger);
}

void mf::ActionControl::drop_trigger_pending(mw::InputTriggerV1 const* trigger)
{
    pending_triggers.erase(trigger);
}

void mf::ActionControl::drop_trigger_immediate(mw::InputTriggerV1 const* trigger)
{
    action.value().drop_trigger(trigger);
}

void mf::ActionControl::add_input_trigger_event(struct wl_resource* trigger)
{
    if (auto const* input_trigger = mw::InputTriggerV1::from(trigger))
    {
        if (!action)
            add_trigger_pending(input_trigger);
        else
            add_trigger_immediate(input_trigger);
    }
}

void mf::ActionControl::drop_input_trigger_event(struct wl_resource* trigger_id)
{
    if (auto const* input_trigger = mw::InputTriggerV1::from(trigger_id))
    {
        if (!action)
            drop_trigger_pending(input_trigger);
        else
            drop_trigger_immediate(input_trigger);
    }
}

void mf::ActionControl::install_action(mw::Weak<mf::InputTriggerActionV1> action)
{
    this->action = action;

    // Add the pending triggers to the composite event filter
    for (auto const* trigger : pending_triggers)
        add_trigger_immediate(trigger);

    // Don't need this anymore
    pending_triggers.clear();
}

mf::InputTriggerData::InputTriggerData(
    std::shared_ptr<msh::TokenAuthority> const& token_authority,
    std::shared_ptr<mi::CompositeEventFilter> const& composite_event_filter) :
    token_authority{token_authority},
    filter{std::make_shared<Filter>(actions)}
{
    composite_event_filter->append(filter);
}

auto mf::InputTriggerData::add_new_action(Token const& token, struct wl_resource* id) -> bool
{
    std::unique_lock lock{mutex};

    erase_expired_entries();

    if (revoked_tokens.contains(token))
    {
        auto const action = new NullInputTriggerActionV1{id};
        action->send_unavailable_event();
        return true;
    }

    if (auto const it = action_controls.find(token); it != action_controls.end())
    {
        auto const action = wayland::make_weak(new InputTriggerActionV1(token_authority, filter->keyboard_state, id));

        auto& [_, action_control] = *it;
        if (action_control)
            action_control.value().install_action(action);

        actions.insert({token, action});
        return true;
    }

    return false;
}

void mf::InputTriggerData::add_new_action_control(struct wl_resource* id)
{
    std::unique_lock lock{mutex};

    erase_expired_entries();

    auto const token = token_authority->issue_token([this](auto const& token) { token_revoked(Token{token}); });

    auto const token_string = static_cast<Token>(token);

    auto const action_control = wayland::make_weak(new ActionControl{token_string, id});
    action_controls.insert({token_string, action_control});

    action_control.value().send_done_event(token_string);
}

auto mf::InputTriggerData::has_trigger(mw::InputTriggerV1 const* trigger) -> bool
{
    std::unique_lock lock{mutex};

    erase_expired_entries();

    // All elements are should be valid now
    return std::ranges::any_of(
        actions, [trigger](auto const pair) { return pair.second.value().has_trigger(trigger); });
}

void mf::InputTriggerData::erase_expired_entries()
{
    std::erase_if(actions, [](auto const& pair) { return !pair.second; });
    std::erase_if(action_controls, [](auto const& pair) { return !pair.second; });
}

// If the token becomes invalid before an action is associated with
// it, we should clean up the action control object.
// When the client tries obtaining the action object using the token,
// they will receive an `unavailable` event.
void mf::InputTriggerData::token_revoked(Token const& token)
{
    std::unique_lock lock{mutex};

    erase_expired_entries();

    if (!actions.contains(token))
        action_controls.erase(token);

    revoked_tokens.add(token);
}

mf::InputTriggerData::Filter::Filter(ActionMap& actions) :
    actions{actions},
    keyboard_state{std::make_shared<KeyboardStateTracker>()}
{
}

bool mf::InputTriggerData::Filter::handle(MirEvent const& event)
{
    if (event.type() != mir_event_type_input)
        return false;

    auto const& input_event = event.to_input();

    if(input_event->input_type() != mir_input_event_type_key)
        return false;

    auto const* key_event = input_event->to_keyboard();
    auto const keysym = key_event->keysym();
    auto const scancode = key_event->scan_code();
    auto const action = key_event->action();

    if (action == mir_keyboard_action_down)
        keyboard_state->on_key_down(keysym, scancode);
    else if (action == mir_keyboard_action_up)
        keyboard_state->on_key_up(keysym, scancode);
    else
        return false;

    for (auto const& [_, action] : actions)
    {
        if (action)
        {
            if (action.value().matches(event))
                return true;
        }
    }

    return false;
}
