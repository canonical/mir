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
#include "mir_toolkit/events/enums.h"

#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>

#include <mir/log.h>

#include <algorithm>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

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

    bool contains(std::initializer_list<MirInputEventModifier> mods) const;

private:
    explicit InputTriggerModifiers(MirInputEventModifiers value);

    static auto compute_required_all(MirInputEventModifiers value) -> MirInputEventModifiers;
    static auto compute_required_any(MirInputEventModifiers value) -> MirInputEventModifiers;
    static auto compute_allowed(MirInputEventModifiers value) -> MirInputEventModifiers;

    MirInputEventModifiers const value;

    /// All of these bits must be present in the event.
    MirInputEventModifiers const required_all;
    /// At least one of these bits must be present in the event.
    MirInputEventModifiers const required_any;
    /// No bits outside these may be present in the event.
    MirInputEventModifiers const allowed;
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


private:
    bool do_process(MirEvent const& event) override;

    uint32_t const keysym;
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

private:
    bool do_process(MirEvent const& event) override;

    uint32_t const scancode;
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
    if (!InputTriggerModifiers::modifiers_match(modifiers, event.to_input()->to_keyboard()->modifiers()))
        return false;

    auto const trigger_mods_contain_shift = modifiers.contains(
        {mir_input_event_modifier_shift, mir_input_event_modifier_shift_left, mir_input_event_modifier_shift_right});

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
    if (!InputTriggerModifiers::modifiers_match(modifiers, event.to_input()->to_keyboard()->modifiers()))
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
// Each entry describes one modifier family: { generic, left, right }.
// left/right are 0 for families that have no sided variants (sym, function).
// clang-format off
constexpr std::array<std::array<MirInputEventModifiers, 3>, 6> modifier_kinds{{
    { mir_input_event_modifier_ctrl,     mir_input_event_modifier_ctrl_left,   mir_input_event_modifier_ctrl_right  },
    { mir_input_event_modifier_alt,      mir_input_event_modifier_alt_left,    mir_input_event_modifier_alt_right   },
    { mir_input_event_modifier_shift,    mir_input_event_modifier_shift_left,  mir_input_event_modifier_shift_right },
    { mir_input_event_modifier_meta,     mir_input_event_modifier_meta_left,   mir_input_event_modifier_meta_right  },
    { mir_input_event_modifier_sym,      MirInputEventModifiers(0),            MirInputEventModifiers(0)            },
    { mir_input_event_modifier_function, MirInputEventModifiers(0),            MirInputEventModifiers(0)            },
}};
// clang-format on

auto InputTriggerModifiers::compute_required_all(MirInputEventModifiers value) -> MirInputEventModifiers
{
    MirInputEventModifiers required_all = 0;
    for (auto const& [generic, left, right] : modifier_kinds)
    {
        MirInputEventModifiers const sides_set = value & (left | right);
        if (sides_set)
            // Specific side requested: require exactly that side.
            required_all |= sides_set;
        // Generic-only: no specific bit is unconditionally required (required_any handles it).
    }
    return required_all;
}

auto InputTriggerModifiers::compute_required_any(MirInputEventModifiers value) -> MirInputEventModifiers
{
    MirInputEventModifiers required_any = 0;
    for (auto const& [generic, left, right] : modifier_kinds)
    {
        bool const has_generic = value & generic;
        bool const has_sides = value & (left | right);
        if (has_generic && !has_sides)
            // Generic requested: at least one side must be present in the event.
            required_any |= left | right;
    }
    return required_any;
}

auto InputTriggerModifiers::compute_allowed(MirInputEventModifiers value) -> MirInputEventModifiers
{
    MirInputEventModifiers allowed = 0;
    for (auto const& [generic, left, right] : modifier_kinds)
    {
        bool const has_generic = value & generic;
        bool const has_sides = value & (left | right);
        if (has_generic && !has_sides)
            // Generic requested: either side (and the generic bit) is permitted.
            allowed |= generic | left | right;
        else if (has_sides)
            // Specific side(s) requested: allow only those sides plus the generic bit.
            allowed |= generic | (value & (left | right));
        // else: family not requested -> allow nothing.
    }
    // Unsided families (sym, function): if set in value, allow exactly that bit.
    allowed |= value & (mir_input_event_modifier_sym | mir_input_event_modifier_function);
    return allowed;
}

InputTriggerModifiers::InputTriggerModifiers(MirInputEventModifiers value) :
    value{value},
    required_all{compute_required_all(value)},
    required_any{compute_required_any(value)},
    allowed{compute_allowed(value)}
{
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

    // Map each protocol bit to its corresponding Mir bit.
    // Sided bits map only to their own Mir bit; the generic bit is not included here
    // because required/allowed logic handles the generic bit independently.
    // clang-format off
    constexpr std::array<std::pair<uint32_t, MirInputEventModifiers>, 14> mappings = {
        std::pair{ PM::alt,         mir_input_event_modifier_alt         },
        std::pair{ PM::alt_left,    mir_input_event_modifier_alt_left    },
        std::pair{ PM::alt_right,   mir_input_event_modifier_alt_right   },

        std::pair{ PM::shift,       mir_input_event_modifier_shift       },
        std::pair{ PM::shift_left,  mir_input_event_modifier_shift_left  },
        std::pair{ PM::shift_right, mir_input_event_modifier_shift_right },

        std::pair{ PM::sym,         mir_input_event_modifier_sym         },
        std::pair{ PM::function,    mir_input_event_modifier_function    },

        std::pair{ PM::ctrl,        mir_input_event_modifier_ctrl        },
        std::pair{ PM::ctrl_left,   mir_input_event_modifier_ctrl_left   },
        std::pair{ PM::ctrl_right,  mir_input_event_modifier_ctrl_right  },

        std::pair{ PM::meta,        mir_input_event_modifier_meta        },
        std::pair{ PM::meta_left,   mir_input_event_modifier_meta_left   },
        std::pair{ PM::meta_right,  mir_input_event_modifier_meta_right  },
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

bool InputTriggerModifiers::modifiers_match(InputTriggerModifiers modifiers, MirInputEventModifiers event_mods)
{
    // Special case: an event with no modifiers only matches if the protocol also specified none.
    if (event_mods == MirInputEventModifier::mir_input_event_modifier_none)
        return modifiers.value == MirInputEventModifier::mir_input_event_modifier_none;

    // All required_all bits must be present, at least one required_any bit must be present
    // (for generic-only families), and no bits outside 'allowed' may be set.
    return (event_mods & modifiers.required_all) == modifiers.required_all
        && (modifiers.required_any == 0 || (event_mods & modifiers.required_any) != 0)
        && (event_mods & ~modifiers.allowed) == 0;
}

bool InputTriggerModifiers::contains(std::initializer_list<MirInputEventModifier> mods) const
{
    return std::ranges::any_of(mods, [this](auto mod) { return (value & mod) == mod; });
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

    mir::log_warning("Non-mw::InputTriggerV1 resource passed to KeyboardTrigger:from");
    return nullptr;
}

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
        std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry) :
        Global{display, Version<1>{}},
        action_group_manager{action_group_manager},
        input_trigger_registry{input_trigger_registry}
    {
    }

    class Instance : public mw::InputTriggerRegistrationManagerV1
    {
    public:
        Instance(
            wl_resource* new_ext_input_trigger_registration_manager_v1,
            std::shared_ptr<mf::ActionGroupManager> const& action_group_manager,
            std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry) :
            mw::InputTriggerRegistrationManagerV1{new_ext_input_trigger_registration_manager_v1, Version<1>{}},
            action_group_manager{action_group_manager},
            input_trigger_registry{input_trigger_registry}
        {
        }

        void register_keyboard_sym_trigger(uint32_t modifiers, uint32_t keysym, struct wl_resource* id) override;
        void register_keyboard_code_trigger(uint32_t modifiers, uint32_t keycode, struct wl_resource* id) override;
        void get_action_control(std::string const& name, struct wl_resource* id) override;

    private:
        std::shared_ptr<mf::ActionGroupManager> const action_group_manager;
        std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
    };

    void bind(wl_resource* new_ext_input_trigger_registration_manager_v1) override
    {
        new Instance{new_ext_input_trigger_registration_manager_v1, action_group_manager, input_trigger_registry};
    }

private:
    std::shared_ptr<mf::ActionGroupManager> const action_group_manager;
    std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
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
    auto* keyboard_trigger = new mf::KeyboardSymTrigger{
        InputTriggerModifiers::from_protocol(modifiers, shift_adjustment),
        keysym,
        input_trigger_registry->keyboard_state_tracker(),
        id};

    if (!input_trigger_registry->register_trigger(keyboard_trigger))
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
        input_trigger_registry->keyboard_state_tracker(),
        id};

    if (!input_trigger_registry->register_trigger(keyboard_trigger))
    {
        mir::log_warning(
            "register_keyboard_code_trigger: %s already registered", keyboard_trigger->to_string().c_str());
        keyboard_trigger->send_failed_event();
    }
    else
        keyboard_trigger->send_done_event();
}
}

auto mf::create_input_trigger_registration_manager_v1(
    wl_display* display,
    std::shared_ptr<ActionGroupManager> const& action_group_manager,
    std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry)
    -> std::shared_ptr<mw::InputTriggerRegistrationManagerV1::Global>
{
    return std::make_shared<InputTriggerRegistrationManagerV1>(
        display, action_group_manager, input_trigger_registry);
}
