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

#include "src/server/frontend_wayland/input_trigger_data.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <format>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using ProtocolModifiers = mw::InputTriggerRegistrationManagerV1::Modifiers;

using namespace testing;

namespace
{

struct ModifierTestParam
{
    uint32_t protocol_modifiers;
    MirInputEventModifiers event_modifiers;
    bool should_match;
    std::string description;
};

// Actually used by GTest
[[maybe_unused]]
void PrintTo(ModifierTestParam const& param, std::ostream* os)
{
    *os << std::format(
        "ModifierTestParam{{\n"
        "  description: {},\n"
        "  protocol_modifiers: {} (0x{:x}),\n"
        "  event_modifiers: {} (0x{:x}),\n"
        "  should_match: {}\n"
        "}}",
        param.description,
        mf::InputTriggerModifiers::from_protocol(param.protocol_modifiers).to_string(),
        param.protocol_modifiers,
        mf::InputTriggerModifiers{param.event_modifiers}.to_string(),
        param.event_modifiers,
        param.should_match);
}

class ProtocolModifierMatchParametricTest : public TestWithParam<ModifierTestParam>
{
};

TEST_P(ProtocolModifierMatchParametricTest, modifier_matching)
{
    auto const& param = GetParam();

    auto const protocol_modifiers = mf::InputTriggerModifiers::from_protocol(param.protocol_modifiers);
    bool result = mf::KeyboardTrigger::modifiers_match(
        protocol_modifiers, mf::InputTriggerModifiers{param.event_modifiers});

    EXPECT_EQ(result, param.should_match);
}

INSTANTIATE_TEST_SUITE_P(
    ShiftModifierVariants,
    ProtocolModifierMatchParametricTest,
    Values(
        ModifierTestParam{
            ProtocolModifiers::shift,
            MirInputEventModifiers(mir_input_event_modifier_shift | mir_input_event_modifier_shift_left),
            true,
            "generic_shift_matches_shift_left"},
        ModifierTestParam{
            ProtocolModifiers::shift,
            MirInputEventModifiers(mir_input_event_modifier_shift | mir_input_event_modifier_shift_right),
            true,
            "generic_shift_matches_shift_right"},
        ModifierTestParam{
            ProtocolModifiers::shift,
            MirInputEventModifiers(
                mir_input_event_modifier_shift | mir_input_event_modifier_shift_left |
                mir_input_event_modifier_shift_right),
            true,
            "generic_shift_matches_both_shift_sides"},
        ModifierTestParam{
            ProtocolModifiers::shift_left,
            MirInputEventModifiers(mir_input_event_modifier_shift | mir_input_event_modifier_shift_left),
            true,
            "specific_shift_left_matches"},
        ModifierTestParam{
            ProtocolModifiers::shift_left,
            MirInputEventModifiers(mir_input_event_modifier_shift | mir_input_event_modifier_shift_right),
            false,
            "specific_shift_left_rejects_shift_right"},
        ModifierTestParam{
            ProtocolModifiers::shift_left,
            MirInputEventModifiers(
                mir_input_event_modifier_shift | mir_input_event_modifier_shift_left |
                mir_input_event_modifier_shift_right),
            false,
            "specific_shift_left_rejects_both_shift_sides"},
        ModifierTestParam{
            ProtocolModifiers::shift_left | ProtocolModifiers::shift_right,
            MirInputEventModifiers(
                mir_input_event_modifier_shift | mir_input_event_modifier_shift_left |
                mir_input_event_modifier_shift_right),
            true,
            "both_shift_sides_requires_both"},
        ModifierTestParam{
            ProtocolModifiers::shift_left | ProtocolModifiers::shift_right,
            MirInputEventModifiers(mir_input_event_modifier_shift | mir_input_event_modifier_shift_left),
            false,
            "both_shift_sides_rejects_only_left",
        },
        ModifierTestParam{
            ProtocolModifiers::shift_left | ProtocolModifiers::shift_right,
            MirInputEventModifiers(mir_input_event_modifier_shift | mir_input_event_modifier_shift_right),
            false,
            "both_shift_sides_rejects_only_right"}),
    [](auto const& info) { return info.param.description; });

INSTANTIATE_TEST_SUITE_P(
    AltModifierVariants,
    ProtocolModifierMatchParametricTest,
    Values(
        ModifierTestParam{
            ProtocolModifiers::alt,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_left),
            true,
            "generic_alt_matches_alt_left"
        },
        ModifierTestParam{
            ProtocolModifiers::alt,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_right),
            true,
            "generic_alt_matches_alt_right"
        },
        ModifierTestParam{
            ProtocolModifiers::alt,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_left |
                                  mir_input_event_modifier_alt_right),
            true,
            "generic_alt_matches_both_alt_sides"
        },
        ModifierTestParam{
            ProtocolModifiers::alt_left,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_left),
            true,
            "specific_alt_left_matches"
        },
        ModifierTestParam{
            ProtocolModifiers::alt_left,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_right),
            false,
            "specific_alt_left_rejects_alt_right"
        },
        ModifierTestParam{
            ProtocolModifiers::alt_left,
            MirInputEventModifiers(
                mir_input_event_modifier_alt | mir_input_event_modifier_alt_left |
                mir_input_event_modifier_alt_right),
            false,
            "specific_alt_left_rejects_both_alt_sides"},
        ModifierTestParam{
            ProtocolModifiers::alt_left | ProtocolModifiers::alt_right,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_left |
                                  mir_input_event_modifier_alt_right),
            true,
            "both_alt_sides_requires_both"
        },
        ModifierTestParam{
            ProtocolModifiers::alt_left | ProtocolModifiers::alt_right,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_left),
            false,
            "both_alt_sides_rejects_only_left"
        },
        ModifierTestParam{
            ProtocolModifiers::alt_left | ProtocolModifiers::alt_right,
            MirInputEventModifiers(mir_input_event_modifier_alt | mir_input_event_modifier_alt_right),
            false,
            "both_alt_sides_rejects_only_right"}),
    [](auto const& info) { return info.param.description; }
);

INSTANTIATE_TEST_SUITE_P(
    CtrlModifierVariants,
    ProtocolModifierMatchParametricTest,
    Values(
        ModifierTestParam{
            ProtocolModifiers::ctrl,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left),
            true,
            "generic_ctrl_matches_ctrl_left"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_right),
            true,
            "generic_ctrl_matches_ctrl_right"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left |
                                  mir_input_event_modifier_ctrl_right),
            true,
            "generic_ctrl_matches_both_ctrl_sides"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl_left,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left),
            true,
            "specific_ctrl_left_matches"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl_left,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_right),
            false,
            "specific_ctrl_left_rejects_ctrl_right"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl_left,
            MirInputEventModifiers(
                mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left |
                mir_input_event_modifier_ctrl_right),
            false,
            "specific_ctrl_left_rejects_both_ctrl_sides"},
        ModifierTestParam{
            ProtocolModifiers::ctrl_left | ProtocolModifiers::ctrl_right,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left |
                                  mir_input_event_modifier_ctrl_right),
            true,
            "both_ctrl_sides_requires_both"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl_left | ProtocolModifiers::ctrl_right,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left),
            false,
            "both_ctrl_sides_rejects_only_left"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl_left | ProtocolModifiers::ctrl_right,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_right),
            false,
            "both_ctrl_sides_rejects_only_right"}),
    [](auto const& info) { return info.param.description; }
);

INSTANTIATE_TEST_SUITE_P(
    MetaModifierVariants,
    ProtocolModifierMatchParametricTest,
    Values(
        ModifierTestParam{
            ProtocolModifiers::meta,
            MirInputEventModifiers(mir_input_event_modifier_meta | mir_input_event_modifier_meta_left),
            true,
            "generic_meta_matches_meta_left"
        },
        ModifierTestParam{
            ProtocolModifiers::meta,
            MirInputEventModifiers(mir_input_event_modifier_meta | mir_input_event_modifier_meta_right),
            true,
            "generic_meta_matches_meta_right"
        },
        ModifierTestParam{
            ProtocolModifiers::meta,
            MirInputEventModifiers(mir_input_event_modifier_meta | mir_input_event_modifier_meta_left |
                                  mir_input_event_modifier_meta_right),
            true,
            "generic_meta_matches_both_meta_sides"
        },
        ModifierTestParam{
            ProtocolModifiers::meta_left,
            MirInputEventModifiers(mir_input_event_modifier_meta | mir_input_event_modifier_meta_left),
            true,
            "specific_meta_left_matches"
        },
        ModifierTestParam{
            ProtocolModifiers::meta_left,
            MirInputEventModifiers(mir_input_event_modifier_meta | mir_input_event_modifier_meta_right),
            false,
            "specific_meta_left_rejects_meta_right"
        },
        ModifierTestParam{
            ProtocolModifiers::meta_left | ProtocolModifiers::meta_right,
            MirInputEventModifiers(mir_input_event_modifier_meta | mir_input_event_modifier_meta_left |
                                  mir_input_event_modifier_meta_right),
            true,
            "both_meta_sides_requires_both"
        },
        ModifierTestParam{
            ProtocolModifiers::meta_left | ProtocolModifiers::meta_right,
            MirInputEventModifiers(mir_input_event_modifier_meta | mir_input_event_modifier_meta_left),
            false,
            "both_meta_sides_rejects_only_left"
        }
    ),
    [](auto const& info) { return info.param.description; }
);

INSTANTIATE_TEST_SUITE_P(
    SpecialModifiers,
    ProtocolModifierMatchParametricTest,
    Values(
        ModifierTestParam{
            ProtocolModifiers::sym,
            mir_input_event_modifier_sym,
            true,
            "sym_modifier_matches"
        },
        ModifierTestParam{
            ProtocolModifiers::function,
            mir_input_event_modifier_function,
            true,
            "function_modifier_matches"
        }
    ),
    [](auto const& info) { return info.param.description; }
);

INSTANTIATE_TEST_SUITE_P(
    EdgeCases,
    ProtocolModifierMatchParametricTest,
    Values(
        ModifierTestParam{
            0,
            mir_input_event_modifier_none,
            true,
            "no_modifiers_matches_clean_event"
        },
        ModifierTestParam{
            0,
            mir_input_event_modifier_ctrl,
            false,
            "no_modifiers_rejects_event_with_modifiers"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl | ProtocolModifiers::alt,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left |
                                  mir_input_event_modifier_alt | mir_input_event_modifier_alt_right),
            true,
            "multiple_generic_modifiers"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left |
                                  mir_input_event_modifier_shift),
            false,
            "rejects_extra_unwanted_modifiers"
        },
        ModifierTestParam{
            ProtocolModifiers::ctrl | ProtocolModifiers::alt,
            MirInputEventModifiers(mir_input_event_modifier_ctrl | mir_input_event_modifier_ctrl_left),
            false,
            "rejects_missing_required_modifiers"
        }
    ),
    [](auto const& info) { return info.param.description; }
);
}  // namespace
