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

#include "input_trigger_v1.h"
#include "input_trigger_registry.h"

#include <mir/log.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

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

bool mf::InputTriggerModifiers::modifiers_match(mf::InputTriggerModifiers modifiers, mf::InputTriggerModifiers event_mods)
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

void mf::InputTriggerV1::associate_with_action_group(std::shared_ptr<frontend::ActionGroup> action_group)
{
    this->action_group = action_group;
}

void mf::InputTriggerV1::unassociate_with_action_group(std::shared_ptr<frontend::ActionGroup> action_group)
{
    if (this->action_group == action_group)
        this->action_group.reset();
}

bool mf::InputTriggerV1::active() const
{
    if (action_group)
        return action_group->any_trigger_active();

    return false;
}

void mf::InputTriggerV1::begin(std::string const& token, uint32_t wayland_timestamp)
{
    if (action_group)
        action_group->begin(token, wayland_timestamp);
}

void mf::InputTriggerV1::end(std::string const& token, uint32_t wayland_timestamp)
{
    if (action_group)
        action_group->end(token, wayland_timestamp);
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
