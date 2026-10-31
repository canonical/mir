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

#include "src/server/frontend_wayland/input_trigger_registration_v1.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using namespace testing;

namespace
{
auto const super_trigger = mf::InputTriggerModifiers{mir_input_event_modifier_meta, mir_input_event_modifier_meta};
auto const alt_trigger = mf::InputTriggerModifiers{mir_input_event_modifier_alt, mir_input_event_modifier_alt};
}

struct InputTriggerModifiersTest : Test
{
};

// Regression test for https://github.com/canonical/mir/issues/5248
//
// A "Super + d" keybind must not swallow "Shift + d" (used to type a capital
// "D"). Previously event_modifiers_are_superset() only checked for an extra
// modifier group and ignored whether the trigger's required modifiers were
// present, so a lone Shift satisfied the "superset consume" path.
TEST_F(InputTriggerModifiersTest, shift_only_is_not_a_superset_of_super_trigger)
{
    EXPECT_FALSE(
        mf::InputTriggerModifiers::event_modifiers_are_superset(super_trigger, mir_input_event_modifier_shift));
}

TEST_F(InputTriggerModifiersTest, shift_only_does_not_match_super_trigger)
{
    EXPECT_FALSE(mf::InputTriggerModifiers::modifiers_match(super_trigger, mir_input_event_modifier_shift));
}

TEST_F(InputTriggerModifiersTest, super_plus_extra_modifier_is_a_superset)
{
    EXPECT_TRUE(
        mf::InputTriggerModifiers::event_modifiers_are_superset(
            super_trigger, mir_input_event_modifier_meta | mir_input_event_modifier_shift));
}

TEST_F(InputTriggerModifiersTest, exact_super_is_not_a_superset)
{
    EXPECT_FALSE(mf::InputTriggerModifiers::event_modifiers_are_superset(super_trigger, mir_input_event_modifier_meta));
}

TEST_F(InputTriggerModifiersTest, exact_super_matches_super_trigger)
{
    EXPECT_TRUE(mf::InputTriggerModifiers::modifiers_match(super_trigger, mir_input_event_modifier_meta));
}

// The superset path still exists to gracefully consume events for a held combo
// when an unrelated modifier is added (e.g. "Alt + s" held, then Shift pressed).
TEST_F(InputTriggerModifiersTest, held_combo_with_added_modifier_is_a_superset)
{
    EXPECT_TRUE(
        mf::InputTriggerModifiers::event_modifiers_are_superset(
            alt_trigger, mir_input_event_modifier_alt | mir_input_event_modifier_shift));
}

TEST_F(InputTriggerModifiersTest, added_modifier_without_required_is_not_a_superset)
{
    EXPECT_FALSE(mf::InputTriggerModifiers::event_modifiers_are_superset(alt_trigger, mir_input_event_modifier_shift));
}

TEST_F(InputTriggerModifiersTest, implicit_shift_adjustment_preserves_explicit_shift_side_restrictions)
{
    // Passing true models a lowercase keysym normalized to uppercase during
    // registration.
    using PM = mw::InputTriggerRegistrationManagerV1::Modifiers;
    auto const left_trigger = mf::InputTriggerModifiers::from_protocol(PM::shift_left, true);
    auto const right_trigger = mf::InputTriggerModifiers::from_protocol(PM::shift_right, true);
    auto const left_shift = mir_input_event_modifier_shift | mir_input_event_modifier_shift_left;
    auto const right_shift = mir_input_event_modifier_shift | mir_input_event_modifier_shift_right;
    auto const both_shifts = mir_input_event_modifier_shift |
        mir_input_event_modifier_shift_left |
        mir_input_event_modifier_shift_right;

    EXPECT_TRUE(mf::InputTriggerModifiers::modifiers_match(left_trigger, left_shift));
    EXPECT_FALSE(mf::InputTriggerModifiers::modifiers_match(left_trigger, both_shifts));
    EXPECT_TRUE(mf::InputTriggerModifiers::modifiers_match(right_trigger, right_shift));
    EXPECT_FALSE(mf::InputTriggerModifiers::modifiers_match(right_trigger, both_shifts));
}
