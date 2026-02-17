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
#include <mir/log.h>
#include <mir_toolkit/events/enums.h>

#include <ranges>
#include <wayland-server-core.h>

#include <algorithm>
#include <string>
#include <vector>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mw = mir::wayland;

class mf::InputTriggerData::TokenData::ActionControl : public mw::InputTriggerActionControlV1
{
public:
    ActionControl(
        TriggerList& triggers,
        ActionGroup& action_group,
        TriggerList& pending_triggers,
        std::shared_ptr<InputTriggerLifetimeTracker> const&,
        struct wl_resource* id);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

private:
    void add_trigger_pending(frontend::InputTriggerV1 const* trigger);
    void add_trigger_immediate(frontend::InputTriggerV1 const* trigger);
    void drop_trigger_pending(frontend::InputTriggerV1 const* trigger);
    void drop_trigger_immediate(frontend::InputTriggerV1 const* trigger);

    TriggerList& triggers;
    ActionGroup& action_group;
    TriggerList& pending_triggers;

    std::shared_ptr<InputTriggerLifetimeTracker> const lifetime_tracker;
};

mf::InputTriggerModifiers::InputTriggerModifiers(MirInputEventModifiers value) :
    value{value}
{
}

auto mf::InputTriggerModifiers::raw_value() const -> MirInputEventModifiers
{
    return value;
}


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

void mf::RecentTokens::add(std::string_view token)
{
    *current = token;

    if (++current == tokens.end())
    {
        current = tokens.begin();
    }
}

auto mf::RecentTokens::contains(std::string_view token) const -> bool
{
    return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
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

auto mf::InputTriggerV1::from(struct wl_resource* resource) -> InputTriggerV1*
{
    if (auto* frontend_trigger = wayland::InputTriggerV1::from(resource))
        return dynamic_cast<InputTriggerV1*>(frontend_trigger);

    mir::log_error("Non-InputTriggerV1 resource passed to InputTriggerV1::from");
    return nullptr;
}

bool mf::InputTriggerV1::is_same_trigger(KeyboardSymTrigger const*) const
{
    return false;
}

bool mf::InputTriggerV1::is_same_trigger(KeyboardCodeTrigger const*) const
{
    return false;
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
    MirEvent const& ev, KeyboardStateTracker const& keyboard_state) const -> bool
{
    auto const event_modifiers = InputTriggerModifiers{ev.to_input()->to_keyboard()->modifiers()};
    if (!KeyboardTrigger::modifiers_match(modifiers, event_modifiers))
        return false;

    auto const mods_value = modifiers.raw_value();
    auto const trigger_mods_contain_shift =
        ((mods_value & mir_input_event_modifier_shift) | (mods_value & mir_input_event_modifier_shift_left) |
         (mods_value & mir_input_event_modifier_shift_right)) != 0;

    return keyboard_state.keysym_is_pressed(keysym, trigger_mods_contain_shift);
}

bool mf::KeyboardSymTrigger::is_same_trigger(InputTriggerV1 const* other) const
{
    return other->is_same_trigger(this);
}

bool mf::KeyboardSymTrigger::is_same_trigger(KeyboardSymTrigger const* other) const
{
    return other && keysym == other->keysym && modifiers == other->modifiers;
}

mf::KeyboardCodeTrigger::KeyboardCodeTrigger(
    InputTriggerModifiers modifiers, uint32_t scancode, struct wl_resource* id) :
    KeyboardTrigger{modifiers, id},
    scancode{scancode}
{
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
    MirEvent const& ev, KeyboardStateTracker const& keyboard_state) const -> bool
{
    auto const event_modifiers = InputTriggerModifiers{ev.to_input()->to_keyboard()->modifiers()};
    if (!KeyboardTrigger::modifiers_match(modifiers, event_modifiers))
        return false;

    return keyboard_state.scancode_is_pressed(scancode);
}

bool mf::KeyboardCodeTrigger::is_same_trigger(InputTriggerV1 const* other) const
{
    return other->is_same_trigger(this);
}

bool mf::KeyboardCodeTrigger::is_same_trigger(KeyboardCodeTrigger const* other) const
{
    return other && scancode == other->scancode && modifiers == other->modifiers;
}

class mf::InputTriggerLifetimeTracker
{
public:
    InputTriggerLifetimeTracker(std::function<void()> on_destroy);
    ~InputTriggerLifetimeTracker();

private:
    std::function<void()> on_destroy;
};

mf::InputTriggerLifetimeTracker::InputTriggerLifetimeTracker(std::function<void()> on_destroy) :
    on_destroy{std::move(on_destroy)}
{
}

mf::InputTriggerLifetimeTracker::~InputTriggerLifetimeTracker()
{
    on_destroy();
}

mf::InputTriggerActionV1::InputTriggerActionV1(wl_resource* id, std::shared_ptr<InputTriggerLifetimeTracker> const& foo) :
    wayland::InputTriggerActionV1{id, Version<1>{}},
    lifetime_tracker{foo}
{
}

void mf::InputTriggerData::TokenData::ActionGroup::add(wayland::Weak<frontend::InputTriggerActionV1 const> action)
{
    actions.push_back(action);
}

auto mf::InputTriggerData::TokenData::ActionGroup::began() const -> bool
{
    return began_;
}

void mf::InputTriggerData::TokenData::ActionGroup::end(std::string const& activation_token, uint32_t wayland_timestamp)
{
    for (auto const& action : actions)
    {
        if (!action)
            continue;
        action.value().send_end_event(wayland_timestamp, activation_token);
    }

    began_ = false;
}

void mf::InputTriggerData::TokenData::ActionGroup::begin(
    std::string const& activation_token, uint32_t wayland_timestamp)
{
    for (auto const& action : actions)
    {
        if (!action)
            continue;
        action.value().send_begin_event(wayland_timestamp, activation_token);
    }

    began_ = true;
}
bool mf::InputTriggerData::TokenData::ActionGroup::empty() const
{
    return actions.empty();
}

void mf::InputTriggerData::TokenData::erase(Token const& token)
{
    entries.erase(token);
}

mf::InputTriggerData::TokenData::ActionControl::ActionControl(
    TriggerList& triggers,
    ActionGroup& action_group,
    TriggerList& pending_triggers,
    std::shared_ptr<InputTriggerLifetimeTracker> const& foo,
    struct wl_resource* id) :
    mw::InputTriggerActionControlV1{id, Version<1>{}},
    triggers{triggers},
    action_group{action_group},
    pending_triggers{pending_triggers},
    lifetime_tracker{foo}
{
}

void mf::InputTriggerData::TokenData::ActionControl::add_trigger_pending(mf::InputTriggerV1 const* trigger)
{
    pending_triggers.push_back(wayland::make_weak(trigger));
}

void mf::InputTriggerData::TokenData::ActionControl::add_trigger_immediate(mf::InputTriggerV1 const* trigger)
{
    triggers.push_back(wayland::make_weak(trigger));
}

void mf::InputTriggerData::TokenData::ActionControl::drop_trigger_pending(mf::InputTriggerV1 const* trigger)
{
    std::erase(pending_triggers, wayland::make_weak(trigger));
}

void mf::InputTriggerData::TokenData::ActionControl::drop_trigger_immediate(mf::InputTriggerV1 const* trigger)
{
    std::erase(triggers, wayland::make_weak(trigger));
}

void mf::InputTriggerData::TokenData::ActionControl::add_input_trigger_event(struct wl_resource* trigger)
{
    if (auto const* input_trigger = mf::InputTriggerV1::from(trigger))
    {
        if(action_group.empty())
            add_trigger_pending(input_trigger);
        else
            add_trigger_immediate(input_trigger);
    }
}

void mf::InputTriggerData::TokenData::ActionControl::drop_input_trigger_event(struct wl_resource* trigger_id)
{
    if (auto const* input_trigger = mf::InputTriggerV1::from(trigger_id))
    {
        if (action_group.empty())
            drop_trigger_pending(input_trigger);
        else
            drop_trigger_immediate(input_trigger);
    }
}

mf::InputTriggerData::InputTriggerData(
    std::shared_ptr<msh::TokenAuthority> const& token_authority, Executor& wayland_executor) :
    token_authority{token_authority},
    wayland_executor{wayland_executor}
{
}

auto mf::InputTriggerData::add_new_action(Token const& token, struct wl_resource* id) -> bool
{
    if (revoked_tokens.contains(token))
    {
        auto const action = new NullInputTriggerActionV1{id};
        action->send_unavailable_event();
        return true;
    }

    if (token_data.is_valid(token))
    {
        token_data.add_action(token, id);
        return true;
    }

    return false;
}

void mf::InputTriggerData::add_new_action_control(struct wl_resource* id)
{
    auto const token = token_authority->issue_token(
        [this](auto const& token) { wayland_executor.spawn([this, token] { token_revoked(Token{token}); }); });

    auto const token_string = static_cast<Token>(token);

    auto const lifetime_tracker =
        std::make_shared<InputTriggerLifetimeTracker>([this, token_string] { token_data.erase(token_string); });
    token_data.add_action_control(token_string, lifetime_tracker, id);
}

auto mf::InputTriggerData::has_trigger(mf::InputTriggerV1 const* trigger) -> bool
{
    return token_data.has_trigger(trigger);
}

// If the token becomes invalid before an action or action control object is
// associated with it, we should clean up the token data corresponing to it.
// When the client tries obtaining the action object using the token, they will
// receive an `unavailable` event.
void mf::InputTriggerData::token_revoked(Token const& token)
{
    revoked_tokens.add(token);
}

bool mf::InputTriggerData::matches(MirEvent const& event)
{
    if (event.type() != mir_event_type_input)
        return false;

    auto const& input_event = event.to_input();

    if (input_event->input_type() != mir_input_event_type_key)
        return false;

    auto const* key_event = input_event->to_keyboard();
    auto const keysym = key_event->keysym();
    auto const scancode = key_event->scan_code();
    auto const action = key_event->action();

    if (action == mir_keyboard_action_down)
        keyboard_state.on_key_down(keysym, scancode);
    else if (action == mir_keyboard_action_up)
        keyboard_state.on_key_up(keysym, scancode);
    else
        return false;

    return token_data.matches(event, keyboard_state, *token_authority);
}

void mf::InputTriggerData::erase(Token const& token)
{
    token_data.erase(token);
}

bool mf::InputTriggerData::TokenData::is_valid(Token const& token) const
{
    return entries.contains(token);
}

void mf::InputTriggerData::TokenData::add_action(Token const& token, struct wl_resource* id)
{
    auto& td = entries[token];

    auto const action = wayland::make_weak(new InputTriggerActionV1 const{id, td.lifetime_tracker.lock()});
    td.action_group.add(action);

    td.trigger_list.insert(td.trigger_list.end(), td.pending_triggers.begin(), td.pending_triggers.end());
    td.pending_triggers.clear();
}

void mf::InputTriggerData::TokenData::add_action_control(
    Token const& token, std::shared_ptr<InputTriggerLifetimeTracker> const& lifetime_tracker, struct wl_resource* id)
{
    auto& [trigger_list, action_group, pending_triggers, weak_lifetime_tracker] = entries[token];

    weak_lifetime_tracker = lifetime_tracker;
    auto const action_control =
        wayland::make_weak(new ActionControl{trigger_list, action_group, pending_triggers, lifetime_tracker, id});
    action_control.value().send_done_event(token);
}

bool mf::InputTriggerData::TokenData::has_trigger(frontend::InputTriggerV1 const* trigger) const
{
    auto const entry_has_trigger = [](auto const& entry, auto const* trigger)
    {
        return std::ranges::any_of(
            entry.trigger_list,
            [trigger](auto const& action_trigger) { return action_trigger.value().is_same_trigger(trigger); });
    };

    return std::ranges::any_of(
        std::ranges::views::values(entries), [&](auto const& entry) { return entry_has_trigger(entry, trigger); });
}

bool mf::InputTriggerData::TokenData::matches(
    MirEvent const& event, KeyboardStateTracker const& keyboard_state, shell::TokenAuthority& token_authority)
{
    return std::ranges::any_of(
        std::ranges::views::values(entries),
        [&](auto& entry)
        {
            auto const matched = std::ranges::any_of(
                entry.trigger_list,
                [&](auto const& trigger) { return trigger && trigger.value().matches(event, keyboard_state); });

            if (!matched)
            {
                if (entry.action_group.began())
                {
                    auto const activation_token = std::string{token_authority.issue_token(std::nullopt)};
                    auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());
                    entry.action_group.end(activation_token, timestamp);
                }

                return false;
            }

            if (!entry.action_group.began())
            {
                auto const activation_token = std::string{token_authority.issue_token(std::nullopt)};
                auto const timestamp = mir_input_event_get_wayland_timestamp(event.to_input());
                entry.action_group.begin(activation_token, timestamp);
                return true;
            }

            return false;
        });
}
