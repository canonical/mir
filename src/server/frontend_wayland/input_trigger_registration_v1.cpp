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

#include "input_trigger_registry.h"
#include "keyboard_state_tracker.h"
#include "mir_toolkit/events/enums.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>

#include <mir/log.h>

#include <xkbcommon/xkbcommon.h>

#include <algorithm>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using Trigger = mf::InputTriggerRegistry::Trigger;
using ActionGroup = mf::InputTriggerRegistry::ActionGroup;
using ActionGroupManager = mf::InputTriggerRegistry::ActionGroupManager;

// Putting these at the very top because KeyboardSymTrigger and
// KeyboardCodeTrigger need to use them.
namespace
{
/// Strong type representing modifier flags used internally by Mir.
class InputTriggerModifiers
{
public:
    /// Convert to string for debugging
    auto to_string() const -> std::string;

    /// Explicit conversion from ProtocolModifiers
    static auto from_protocol(uint32_t protocol_mods) -> InputTriggerModifiers;

    /// Explicit conversion from ProtocolModifiers with keysym for shift
    /// adjustment.
    ///
    /// `protocol_mods` is a mask containing the protocol modifier flags (e.g.
    /// Shift, Ctrl, Alt) as defined in ext_input_trigger_registration_v1. A
    /// client could request a trigger with an uppercase letter keysym, but not
    /// provide a shift modifier. "Ctrl + E" for example. The keysym
    /// corresponding to "E" only appears in input events if Shift is pressed.
    /// But since the client did not specify Shift in their original request,
    /// the protocol modifier mask will not contain Shift, and the event
    /// modifier mask will contain Shift, and the trigger won't match. To
    /// account for this, we patch the protocol modifiers at registration time.
    static auto from_protocol(uint32_t protocol_mods, bool shift_adjustment) -> InputTriggerModifiers;

    bool operator==(InputTriggerModifiers const& other) const = default;

    static auto modifiers_match(InputTriggerModifiers modifiers, MirInputEventModifiers event_mods) -> bool;
    static auto event_modifiers_are_superset(InputTriggerModifiers modifiers, MirInputEventModifiers event_mods) -> bool;

private:
    explicit InputTriggerModifiers(MirInputEventModifiers required, MirInputEventModifiers allowed);

    MirInputEventModifiers const required;
    MirInputEventModifiers const allowed;
};

class KeyboardTrigger : public Trigger, public mw::InputTriggerV1
{
public:
    KeyboardTrigger(
        InputTriggerModifiers modifiers,
        std::shared_ptr<mf::KeyboardStateTracker const> const& keyboard_state_tracker,
        struct wl_resource* id);

    static auto from(struct wl_resource* resource) -> KeyboardTrigger*;

    InputTriggerModifiers const modifiers;
    std::shared_ptr<mf::KeyboardStateTracker const> const keyboard_state_tracker;

private:
    virtual auto is_active() const -> bool override;
    auto check_transition(MirEvent const& event) -> Transition override;

    virtual bool check_pressed(MirEvent const& event) const = 0;
    virtual bool event_is_for_trigger_key(MirEvent const& event) const = 0;

    bool active{false};
};

auto KeyboardTrigger::is_active() const -> bool
{
    return active;
}

auto KeyboardTrigger::check_transition(MirEvent const& event) -> Transition
{
    if (event.type() != mir_event_type_input || event.to_input()->input_type() != mir_input_event_type_key)
        return Transition::pass;

    // Remove caps, num, and scroll lock from the event modifiers, since those
    // are not part of the trigger specification and would prevent matches.
    auto constexpr modifiers_to_ignore =
        mir_input_event_modifier_caps_lock | mir_input_event_modifier_num_lock | mir_input_event_modifier_scroll_lock;
    auto const& keyboard_event = *event.to_input()->to_keyboard();
    auto const event_mods = keyboard_event.modifiers() & ~modifiers_to_ignore;

    auto const event_is_for_trigger_key = this->event_is_for_trigger_key(event);
    // For "Alt + s", handles the case where the user completes the combo, then
    // presses "shift". Consumes up and down events of "s" until shift is let
    // go.
    if (InputTriggerModifiers::event_modifiers_are_superset(modifiers, event_mods) && event_is_for_trigger_key)
        return Transition::consume;

    if (!InputTriggerModifiers::modifiers_match(modifiers, event_mods))
    {
        if (!active)
            return Transition::pass;

        active = false;
        return Transition::deactivated;
    }

    auto const pressed = check_pressed(event);
    if (active && !pressed)
    {
        active = false;
        return Transition::deactivated;
    }
    else if (!active && pressed)
    {
        // If we matched because of a key release (like Alt + Shift + s
        // matching Alt + s on shift release), don't activate.
        if (keyboard_event.action() == mir_keyboard_action_up)
            return Transition::pass;

        active = true;
        return Transition::activated;
    }
    else
    {
        return Transition::pass;
    }
}

struct keysym_to_string
{
    keysym_to_string(uint32_t keysym)
    {
        if (xkb_keysym_get_name(keysym, buf, sizeof buf) >= 0)
            value = buf;
    }

    char const* operator()() const
    {
        return value;
    }

private:
    char buf[64] = {};
    char const* value = "unknown";
};
}

class mf::KeyboardSymTrigger : public KeyboardTrigger
{
public:
    KeyboardSymTrigger(
        InputTriggerModifiers modifiers,
        uint32_t keysym,
        std::shared_ptr<mf::KeyboardStateTracker const> const& keyboard_state_tracker,
        struct wl_resource* id);

    bool is_same_trigger(Trigger const* other) const override;
    bool is_same_trigger(KeyboardSymTrigger const* other) const override;

private:
    bool check_pressed(MirEvent const& event) const override;
    bool event_is_for_trigger_key(MirEvent const& event) const override;

    uint32_t const keysym;
};

class mf::KeyboardCodeTrigger : public KeyboardTrigger
{
public:
    KeyboardCodeTrigger(
        InputTriggerModifiers modifiers,
        uint32_t scancode,
        std::shared_ptr<mf::KeyboardStateTracker const> const& keyboard_state_tracker,
        struct wl_resource* id);

    bool is_same_trigger(Trigger const* other) const override;
    bool is_same_trigger(KeyboardCodeTrigger const* other) const override;

private:
    bool check_pressed(MirEvent const& event) const override;
    bool event_is_for_trigger_key(MirEvent const& event) const override;

    uint32_t const scancode;
};

mf::KeyboardSymTrigger::KeyboardSymTrigger(
    InputTriggerModifiers modifiers,
    uint32_t keysym,
    std::shared_ptr<mf::KeyboardStateTracker const> const& keyboard_state_tracker,
    struct wl_resource* id) :
    KeyboardTrigger{modifiers, keyboard_state_tracker, id},
    keysym{keysym}
{
}

bool mf::KeyboardSymTrigger::check_pressed(MirEvent const& event) const
{
    return keyboard_state_tracker->keysym_is_pressed(event.to_input()->device_id(), keysym);
}

bool mf::KeyboardSymTrigger::event_is_for_trigger_key(MirEvent const& event) const
{
    return keyboard_state_tracker->is_same_key(*event.to_input()->to_keyboard(), keysym);
}

bool mf::KeyboardSymTrigger::is_same_trigger(Trigger const* other) const
{
    return other->is_same_trigger(this);
}

bool mf::KeyboardSymTrigger::is_same_trigger(KeyboardSymTrigger const* other) const
{
    return other && keysym == other->keysym && modifiers == other->modifiers;
}

mf::KeyboardCodeTrigger::KeyboardCodeTrigger(
    InputTriggerModifiers modifiers,
    uint32_t scancode,
    std::shared_ptr<mf::KeyboardStateTracker const> const& keyboard_state_tracker,
    struct wl_resource* id) :
    KeyboardTrigger{modifiers, keyboard_state_tracker, id},
    scancode{scancode}
{
}

bool mf::KeyboardCodeTrigger::check_pressed(MirEvent const& event) const
{
    return keyboard_state_tracker->scancode_is_pressed(event.to_input()->device_id(), scancode);
}

bool mf::KeyboardCodeTrigger::event_is_for_trigger_key(MirEvent const& event) const
{
    auto const& keyboard_event = *event.to_input()->to_keyboard();
    return static_cast<uint32_t>(keyboard_event.scan_code()) == scancode;
}

bool mf::KeyboardCodeTrigger::is_same_trigger(Trigger const* other) const
{
    return other->is_same_trigger(this);
}

bool mf::KeyboardCodeTrigger::is_same_trigger(KeyboardCodeTrigger const* other) const
{
    return other && scancode == other->scancode && modifiers == other->modifiers;
}

namespace
{
InputTriggerModifiers::InputTriggerModifiers(MirInputEventModifiers required, MirInputEventModifiers allowed) :
    required{required},
    allowed{allowed}
{
}

auto to_string(MirInputEventModifiers value) -> std::string
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

auto InputTriggerModifiers::to_string() const -> std::string
{
    return "required=" + ::to_string(required) + " allowed=" + ::to_string(allowed);
}

auto InputTriggerModifiers::from_protocol(uint32_t protocol_mods) -> InputTriggerModifiers
{
    return from_protocol(protocol_mods, false);
}

auto InputTriggerModifiers::from_protocol(uint32_t protocol_mods, bool shift_adjustment) -> InputTriggerModifiers
{
    using PM = mw::InputTriggerRegistrationManagerV1::Modifiers;

    if (protocol_mods == 0 && !shift_adjustment)
        return InputTriggerModifiers{mir_input_event_modifier_none, mir_input_event_modifier_none};
    struct ModifierGroup
    {
        uint32_t protocol_generic;
        uint32_t protocol_left;
        uint32_t protocol_right;
        MirInputEventModifiers generic_bit;
        MirInputEventModifiers left_bit;
        MirInputEventModifiers right_bit;
    };

    constexpr std::array<ModifierGroup, 4> sided_groups = {
        ModifierGroup{PM::shift, PM::shift_left, PM::shift_right,
            mir_input_event_modifier_shift, mir_input_event_modifier_shift_left, mir_input_event_modifier_shift_right},
        ModifierGroup{PM::ctrl, PM::ctrl_left, PM::ctrl_right,
            mir_input_event_modifier_ctrl, mir_input_event_modifier_ctrl_left, mir_input_event_modifier_ctrl_right},
        ModifierGroup{PM::alt, PM::alt_left, PM::alt_right,
            mir_input_event_modifier_alt, mir_input_event_modifier_alt_left, mir_input_event_modifier_alt_right},
        ModifierGroup{PM::meta, PM::meta_left, PM::meta_right,
            mir_input_event_modifier_meta, mir_input_event_modifier_meta_left, mir_input_event_modifier_meta_right},
    };

    MirInputEventModifiers required = 0;
    MirInputEventModifiers allowed = 0;

    for (auto const& group : sided_groups)
    {
        bool const wants_generic = protocol_mods & group.protocol_generic;
        bool const wants_left    = protocol_mods & group.protocol_left;
        bool const wants_right   = protocol_mods & group.protocol_right;

        if (wants_generic)
        {
            // The client accepts either side: require only the generic bit
            // (always present in events alongside any side press), and allow
            // all three bits (generic + left + right).
            required |= group.generic_bit;
            allowed  |= group.generic_bit | group.left_bit | group.right_bit;
        }
        else if (wants_left)
        {
            // Only the left side is acceptable: require the left-side bit, and
            // allow both the left-side bit and the generic bit (which is always
            // set alongside any side-specific press).
            required |= group.left_bit;
            allowed  |= group.left_bit | group.generic_bit;
        }
        else if (wants_right)
        {
            required |= group.right_bit;
            allowed  |= group.right_bit | group.generic_bit;
        }
    }

    // Non-sided modifiers: the required bit and allowed bit are identical.
    if (protocol_mods & PM::sym)
    {
        required |= mir_input_event_modifier_sym;
        allowed  |= mir_input_event_modifier_sym;
    }
    if (protocol_mods & PM::function)
    {
        required |= mir_input_event_modifier_function;
        allowed  |= mir_input_event_modifier_function;
    }

    // When shift_adjustment is active, the trigger was registered with an
    // uppercase keysym but without an explicit shift modifier. Shift will be
    // present in the event, so we must allow it without requiring it.
    if (shift_adjustment)
        allowed |= mir_input_event_modifier_shift | mir_input_event_modifier_shift_left | mir_input_event_modifier_shift_right;

    return InputTriggerModifiers{required, allowed};
}

bool InputTriggerModifiers::modifiers_match(InputTriggerModifiers modifiers, MirInputEventModifiers event_mods)
{
    // All required modifier bits must be present in the event.
    bool const all_required_present = (event_mods & modifiers.required) == modifiers.required;

    // No modifier bits outside of the allowed set may be present in the event.
    bool const no_extra_modifiers = (event_mods & ~modifiers.allowed) == 0;

    return all_required_present && no_extra_modifiers;
}

bool InputTriggerModifiers::event_modifiers_are_superset(InputTriggerModifiers modifiers, MirInputEventModifiers event_mods)
{
    // Superset check per modifier group
    constexpr std::array<MirInputEventModifiers, 4> groups = {
        mir_input_event_modifier_alt | mir_input_event_modifier_alt_left | mir_input_event_modifier_alt_right,
        mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left | mir_input_event_modifier_ctrl_right,
        mir_input_event_modifier_shift | mir_input_event_modifier_shift_left | mir_input_event_modifier_shift_right,
        mir_input_event_modifier_meta | mir_input_event_modifier_meta_left | mir_input_event_modifier_meta_right,
    };


    // If the event contains any modifier group that the trigger doesn't, it's a superset
    return std::ranges::any_of(
        groups,
        [&](auto const group)
        {
            auto const trigger_has_group = modifiers.allowed & group;
            auto const event_has_group = event_mods & group;
            return !trigger_has_group && event_has_group;
        });
}

KeyboardTrigger::KeyboardTrigger(
    InputTriggerModifiers modifiers,
    std::shared_ptr<mf::KeyboardStateTracker const> const& keyboard_state_tracker,
    struct wl_resource* id) :
    InputTriggerV1{id, Version<1>{}},
    modifiers{modifiers},
    keyboard_state_tracker{keyboard_state_tracker}
{
}

auto KeyboardTrigger::from(struct wl_resource* resource) -> KeyboardTrigger*
{
    if (auto* input_trigger = mw::InputTriggerV1::from(resource))
        return dynamic_cast<KeyboardTrigger*>(input_trigger);

    mir::log_warning("Non-mw::InputTriggerV1 resource passed to KeyboardTrigger:from");
    return nullptr;
}

class InputTriggerActionControlV1 : public mw::InputTriggerActionControlV1
{
public:
    InputTriggerActionControlV1(std::shared_ptr<ActionGroup> const& action_group, struct wl_resource* id);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

    void cancel() override;
    void destroy() override;

private:
    std::shared_ptr<ActionGroup> const action_group;
};

InputTriggerActionControlV1::InputTriggerActionControlV1(
    std::shared_ptr<ActionGroup> const& action_group, struct wl_resource* id) :
    mw::InputTriggerActionControlV1{id, Version<1>{}},
    action_group{action_group}
{
}

void InputTriggerActionControlV1::add_input_trigger_event(struct wl_resource* trigger)
{
    if (auto* keyboard_trigger = KeyboardTrigger::from(trigger))
    {
        keyboard_trigger->associate_with_action_group(action_group);
    }
}

void InputTriggerActionControlV1::drop_input_trigger_event(struct wl_resource* trigger_id)
{
    if (auto* keyboard_trigger = KeyboardTrigger::from(trigger_id))
    {
        keyboard_trigger->unassociate_with_action_group(action_group);
    }
}

void InputTriggerActionControlV1::cancel()
{
    action_group->cancel();
    delete this;
}

void InputTriggerActionControlV1::destroy()
{
    delete this;
}

class InputTriggerRegistrationManagerV1 : public mw::InputTriggerRegistrationManagerV1::Global
{
public:
    InputTriggerRegistrationManagerV1(
        wl_display* display,
        std::shared_ptr<ActionGroupManager> const& action_group_manager,
        std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry,
        std::shared_ptr<mf::KeyboardStateTracker> const& keyboard_state_tracker) :
        Global{display, Version<1>{}},
        action_group_manager{action_group_manager},
        input_trigger_registry{input_trigger_registry},
        keyboard_state_tracker{keyboard_state_tracker}
    {
    }

    class Instance : public mw::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<ActionGroupManager> const& action_group_manager,
            std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry,
            std::shared_ptr<mf::KeyboardStateTracker> const& keyboard_state_tracker) :
            mw::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            action_group_manager{action_group_manager},
            input_trigger_registry{input_trigger_registry},
            keyboard_state_tracker{keyboard_state_tracker}
        {
            send_capabilities_event(Capability::keyboard);
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        std::shared_ptr<ActionGroupManager> const action_group_manager;
        std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
        std::shared_ptr<mf::KeyboardStateTracker> const keyboard_state_tracker;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{
            new_ext_input_trigger_registration_manager_v1,
            action_group_manager,
            input_trigger_registry,
            keyboard_state_tracker};
    }

private:
    std::shared_ptr<ActionGroupManager> const action_group_manager;
    std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
    std::shared_ptr<mf::KeyboardStateTracker> const keyboard_state_tracker;
};

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    auto const [token, action_group] = action_group_manager->create_new_action_group();
    auto const action_control = new InputTriggerActionControlV1{action_group, id};
    action_control->send_done_event(token);
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_sym_trigger(
    uint32_t modifiers, uint32_t keysym, struct wl_resource* id)
{
    auto const shift_adjustment = (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z);
    auto const protocol_modifiers = InputTriggerModifiers::from_protocol(modifiers, shift_adjustment);
    auto* keyboard_trigger = new mf::KeyboardSymTrigger{
        protocol_modifiers,
        keysym,
        keyboard_state_tracker,
        id};

    if (!input_trigger_registry->register_trigger(keyboard_trigger))
    {
        mir::log_warning(
            "register_keyboard_sym_trigger: KeyboardSymTrigger{client=%p, keysym=%s, modifiers=%s} already "
            "registered",
            (void*)keyboard_trigger->client,
            keysym_to_string(keysym)(),
            protocol_modifiers.to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_code_trigger(
    uint32_t modifiers, uint32_t keycode, struct wl_resource* id)
{
    auto const protocol_modifiers = InputTriggerModifiers::from_protocol(modifiers);
    auto* keyboard_trigger = new mf::KeyboardCodeTrigger{
        protocol_modifiers,
        keycode,
        keyboard_state_tracker,
        id};

    if (!input_trigger_registry->register_trigger(keyboard_trigger))
    {
        // Scancodes are not directly translatable to strings unless you have a
        // keyboard layout, which we don't have here.
        mir::log_warning(
            "register_keyboard_code_trigger: KeyboardCodeTrigger{client=%p, scancode=%d, modifiers=%s} already "
            "registered",
            (void*)keyboard_trigger->client,
            keycode,
            protocol_modifiers.to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}
}

auto mf::create_input_trigger_registration_manager_v1(
    wl_display* display,
    std::shared_ptr<ActionGroupManager> const& action_group_manager,
    std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry,
    std::shared_ptr<KeyboardStateTracker> const& keyboard_state_tracker)
    -> std::shared_ptr<mw::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<InputTriggerRegistrationManagerV1>(
        display, action_group_manager, input_trigger_registry, keyboard_state_tracker);
}
