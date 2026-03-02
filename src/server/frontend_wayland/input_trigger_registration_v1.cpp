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

#include "input_trigger_common.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>

#include <mir/log.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace
{

/// Strong type representing modifier flags used internally by Mir.
class InputTriggerModifiers
{
public:
    /// Explicit construction from MirInputEventModifiers
    explicit InputTriggerModifiers(MirInputEventModifiers value);

    /// Get the raw MirInputEventModifiers value (for internal use)
    auto raw_value() const -> MirInputEventModifiers;

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

    static auto modifiers_match(InputTriggerModifiers modifiers, InputTriggerModifiers event_mods) -> bool;

private:
    MirInputEventModifiers value;
};

class KeyboardTrigger : public mf::InputTrigger, public mw::InputTriggerV1
{
public:
    KeyboardTrigger(
        InputTriggerModifiers modifiers,
        mf::KeyboardStateTracker const& keyboard_state_tracker,
        struct wl_resource* id);

    static auto from(struct wl_resource* resource) -> KeyboardTrigger*;

    InputTriggerModifiers const modifiers;
    mf::KeyboardStateTracker const& keyboard_state_tracker;
};

InputTriggerModifiers::InputTriggerModifiers(MirInputEventModifiers value) :
    value{value}
{
}

auto InputTriggerModifiers::raw_value() const -> MirInputEventModifiers
{
    return value;
}

auto InputTriggerModifiers::to_string() const -> std::string
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

auto InputTriggerModifiers::from_protocol(uint32_t protocol_mods) -> InputTriggerModifiers
{
    return from_protocol(protocol_mods, false);
}

auto InputTriggerModifiers::from_protocol(uint32_t protocol_mods, bool shift_adjustment) -> InputTriggerModifiers
{
    using PM = mw::InputTriggerRegistrationManagerV1::Modifiers;

    if (protocol_mods == 0)
        return InputTriggerModifiers{mir_input_event_modifier_none};

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

bool InputTriggerModifiers::modifiers_match(InputTriggerModifiers modifiers, InputTriggerModifiers event_mods)
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

KeyboardTrigger::KeyboardTrigger(
    InputTriggerModifiers modifiers,
    mf::KeyboardStateTracker const& keyboard_state_tracker,
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

    mir::log_warning("Non-mw::InputTriggerV1 resource passed to KeyboardTriggert:from");
    return nullptr;
}
}

class mf::KeyboardSymTrigger : public KeyboardTrigger
{
public:
    KeyboardSymTrigger(
        InputTriggerModifiers modifiers,
        uint32_t keysym,
        mf::KeyboardStateTracker const& keyboard_state_tracker,
        struct wl_resource* id);

    auto to_string() const -> std::string override;


    bool is_same_trigger(InputTrigger const* other) const override;
    bool is_same_trigger(KeyboardSymTrigger const* other) const override;

    uint32_t const keysym;

private:
    bool do_process(MirEvent const& event) override;
};

class mf::KeyboardCodeTrigger : public KeyboardTrigger
{
public:
    KeyboardCodeTrigger(
        InputTriggerModifiers modifiers,
        uint32_t scancode,
        mf::KeyboardStateTracker const& keyboard_state_tracker,
        struct wl_resource* id);

    auto to_string() const -> std::string override;

    bool is_same_trigger(InputTrigger const* other) const override;
    bool is_same_trigger(KeyboardCodeTrigger const* other) const override;

    uint32_t const scancode;

private:
    bool do_process(MirEvent const& event) override;
};

mf::KeyboardSymTrigger::KeyboardSymTrigger(
    InputTriggerModifiers modifiers,
    uint32_t keysym,
    mf::KeyboardStateTracker const& keyboard_state_tracker,
    struct wl_resource* id) :
    KeyboardTrigger{modifiers, keyboard_state_tracker, id},
    keysym{keysym}
{
}

auto mf::KeyboardSymTrigger::to_string() const -> std::string
{
    return std::format(
        "KeyboardSymTrigger{{client={}, keysym={}, modifiers={}}}",
        static_cast<void*>(wl_resource_get_client(resource)),
        keysym,
        modifiers.to_string());
}

bool mf::KeyboardSymTrigger::do_process(MirEvent const& event)
{
    auto const event_modifiers = InputTriggerModifiers{event.to_input()->to_keyboard()->modifiers()};
    if (!InputTriggerModifiers::modifiers_match(modifiers, event_modifiers))
        return false;

    auto const mods_value = modifiers.raw_value();
    auto const trigger_mods_contain_shift =
        ((mods_value & mir_input_event_modifier_shift) | (mods_value & mir_input_event_modifier_shift_left) |
         (mods_value & mir_input_event_modifier_shift_right)) != 0;

    return keyboard_state_tracker.keysym_is_pressed(keysym, trigger_mods_contain_shift);
}

bool mf::KeyboardSymTrigger::is_same_trigger(InputTrigger const* other) const
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
    mf::KeyboardStateTracker const& keyboard_state_tracker,
    struct wl_resource* id) :
    KeyboardTrigger{modifiers, keyboard_state_tracker, id},
    scancode{scancode}
{
}

auto mf::KeyboardCodeTrigger::to_string() const -> std::string
{
    return std::format(
        "KeyboardCodeTrigger{{client={}, scancode={}, modifiers={}}}",
        static_cast<void*>(wl_resource_get_client(resource)),
        scancode,
        modifiers.to_string());
}

bool mf::KeyboardCodeTrigger::do_process(MirEvent const& event)
{
    auto const event_modifiers = InputTriggerModifiers{event.to_input()->to_keyboard()->modifiers()};
    if (!InputTriggerModifiers::modifiers_match(modifiers, event_modifiers))
        return false;

    return keyboard_state_tracker.scancode_is_pressed(scancode);
}

bool mf::KeyboardCodeTrigger::is_same_trigger(InputTrigger const* other) const
{
    return other->is_same_trigger(this);
}

bool mf::KeyboardCodeTrigger::is_same_trigger(KeyboardCodeTrigger const* other) const
{
    return other && scancode == other->scancode && modifiers == other->modifiers;
}

namespace
{
class InputTriggerActionControlV1 : public mw::InputTriggerActionControlV1
{
public:
    InputTriggerActionControlV1(std::shared_ptr<mf::ActionGroup> const& action_group, struct wl_resource* id);

    void add_input_trigger_event(struct wl_resource* trigger) override;
    void drop_input_trigger_event(struct wl_resource* trigger) override;

private:
    std::shared_ptr<mf::ActionGroup> const action_group;
};

InputTriggerActionControlV1::InputTriggerActionControlV1(
    std::shared_ptr<mf::ActionGroup> const& action_group, struct wl_resource* id) :
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

class InputTriggerRegistrationManagerV1 : public mw::InputTriggerRegistrationManagerV1::Global
{
public:
    InputTriggerRegistrationManagerV1(
        wl_display* display,
        std::shared_ptr<mf::ActionGroupManager> const& action_group_manager,
        std::shared_ptr<mf::KeyboardTriggerRegistry> const& keyboard_trigger_registry) :
        Global{display, Version<1>{}},
        action_group_manager{action_group_manager},
        keyboard_trigger_registry{keyboard_trigger_registry}
    {
    }

    class Instance : public mw::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<mf::ActionGroupManager> const& action_group_manager,
            std::shared_ptr<mf::KeyboardTriggerRegistry> const& keyboard_trigger_registry) :
            mw::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            action_group_manager{action_group_manager},
            keyboard_trigger_registry{keyboard_trigger_registry}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        std::shared_ptr<mf::ActionGroupManager> const action_group_manager;
        std::shared_ptr<mf::KeyboardTriggerRegistry> const keyboard_trigger_registry;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, action_group_manager, keyboard_trigger_registry};
    }

private:
    std::shared_ptr<mf::ActionGroupManager> const action_group_manager;
    std::shared_ptr<mf::KeyboardTriggerRegistry> const keyboard_trigger_registry;
};

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_sym_trigger(
    uint32_t modifiers, uint32_t keysym, struct wl_resource* id)
{
    auto const shift_adjustment = (keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z);
    auto* keyboard_trigger = new mf::KeyboardSymTrigger{
        InputTriggerModifiers::from_protocol(modifiers, shift_adjustment),
        keysym,
        keyboard_trigger_registry->keyboard_state_tracker(),
        id};

    if (!keyboard_trigger_registry->register_trigger(keyboard_trigger))
    {
        mir::log_warning("register_keyboard_sym_trigger: %s already registered", keyboard_trigger->to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

void InputTriggerRegistrationManagerV1::Instance::register_keyboard_code_trigger(
    uint32_t modifiers, uint32_t keycode, struct wl_resource* id)
{
    auto* keyboard_trigger = new mf::KeyboardCodeTrigger{
        InputTriggerModifiers::from_protocol(modifiers),
        keycode,
        keyboard_trigger_registry->keyboard_state_tracker(),
        id};

    if (!keyboard_trigger_registry->register_trigger(keyboard_trigger))
    {
        mir::log_warning(
            "register_keyboard_code_trigger: %s already registered", keyboard_trigger->to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}

// TODO: Store the description string
void InputTriggerRegistrationManagerV1::Instance::get_action_control(std::string const&, struct wl_resource* id)
{
    auto const [token, action_group] = action_group_manager->create_new_action_group();
    auto const action_control = new InputTriggerActionControlV1{action_group, id};
    action_control->send_done_event(token);
}
}

auto mf::create_input_trigger_registration_manager_v1(
    wl_display* display,
    std::shared_ptr<ActionGroupManager> const& action_group_manager,
    std::shared_ptr<KeyboardTriggerRegistry> const& keyboard_trigger_registry)
    -> std::shared_ptr<mw::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<InputTriggerRegistrationManagerV1>(
        display, action_group_manager, keyboard_trigger_registry);
}
