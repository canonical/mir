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

#include "src/server/frontend_wayland/keyboard_state_tracker.h"
#include "src/server/input/default_event_builder.h"

#include <mir/test/doubles/advanceable_clock.h>
#include <mir/test/fake_shared.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{

/// Based on standard US QWERTY layout (PC keyboard / evdev scancodes)

constexpr uint32_t a_scancode = 30;
constexpr uint32_t b_scancode = 48;

constexpr uint32_t ctrl_l_scancode = 37;
constexpr uint32_t shift_l_scancode = 50;
constexpr uint32_t shift_r_scancode = 62;

class KeyboardStateTrackerTest : public Test
{
public:
    auto key_down(uint32_t keysym, uint32_t scancode) -> mir::EventUPtr
    {
        if (keysym == XKB_KEY_Shift_L)
            modifier_state |= mir_input_event_modifier_shift_left;
        if (keysym == XKB_KEY_Shift_R)
            modifier_state |= mir_input_event_modifier_shift_right;

        return event_builder.key_event(std::nullopt, mir_keyboard_action_down, keysym, scancode, modifier_state);
    }

    auto key_up(uint32_t keysym, uint32_t scancode) -> mir::EventUPtr
    {
        if (keysym == XKB_KEY_Shift_L)
            modifier_state &= ~mir_input_event_modifier_shift_left;
        if (keysym == XKB_KEY_Shift_R)
            modifier_state &= ~mir_input_event_modifier_shift_right;

        return event_builder.key_event(std::nullopt, mir_keyboard_action_up, keysym, scancode, modifier_state);
    }

    mtd::AdvanceableClock clock;
    mir::input::DefaultEventBuilder event_builder{MirInputDeviceId{0}, mir::test::fake_shared(clock)};
    mf::KeyboardStateTracker tracker;

    // Track modifier state to emulate Mir's tracking of modifiers
    MirInputEventModifiers modifier_state{0};
};

TEST_F(KeyboardStateTrackerTest, initially_no_keys_pressed)
{
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_FALSE(tracker.scancode_is_pressed(a_scancode));
}

TEST_F(KeyboardStateTrackerTest, tracks_key_down)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_TRUE(tracker.scancode_is_pressed(a_scancode));
}

TEST_F(KeyboardStateTrackerTest, tracks_key_up)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));
    tracker.process(*key_up(XKB_KEY_a, a_scancode));

    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_FALSE(tracker.scancode_is_pressed(a_scancode));
}

TEST_F(KeyboardStateTrackerTest, tracks_multiple_keys)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));
    tracker.process(*key_down(XKB_KEY_b, b_scancode));
    tracker.process(*key_down(XKB_KEY_Control_L, ctrl_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_b));
    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_Control_L));
    EXPECT_TRUE(tracker.scancode_is_pressed(a_scancode));
    EXPECT_TRUE(tracker.scancode_is_pressed(b_scancode));
    EXPECT_TRUE(tracker.scancode_is_pressed(ctrl_l_scancode));
}

TEST_F(KeyboardStateTrackerTest, lowercase_keysym_does_not_match_uppercase)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));

    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_A));
}

TEST_F(KeyboardStateTrackerTest, uppercase_keysym_does_not_match_lowercase)
{
    tracker.process(*key_down(XKB_KEY_A, a_scancode));

    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, pressing_shift_l_promotes_held_lowercase_keysyms_to_uppercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, pressing_shift_l_after_lowercase_key_promotes_held_lowercase_keysym_to_uppercase)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_A));

    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
}
TEST_F(KeyboardStateTrackerTest, pressing_shift_r_promotes_held_lowercase_keysyms_to_uppercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_R, shift_r_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, releasing_shift_l_demotes_held_uppercase_keysyms_to_lowercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_A));
}

TEST_F(KeyboardStateTrackerTest, releasing_shift_r_demotes_held_uppercase_keysyms_to_lowercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_R, shift_r_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_R, shift_r_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_A));
}

TEST_F(KeyboardStateTrackerTest, shift_promotion_does_not_affect_non_alpha_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_Control_L, ctrl_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_Control_L));
}

TEST_F(KeyboardStateTrackerTest, shift_demotion_does_not_affect_non_alpha_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_Control_L, ctrl_l_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_Control_L));
}

TEST_F(KeyboardStateTrackerTest, shift_promotion_affects_all_held_lowercase_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_down(XKB_KEY_B, b_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_A));
    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_B));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_b));
}

TEST_F(KeyboardStateTrackerTest, shift_demotion_affects_all_held_uppercase_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_down(XKB_KEY_B, b_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a));
    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_b));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_B));
}

TEST_F(KeyboardStateTrackerTest, releasing_one_shift_when_both_are_pressed_does_not_demote_uppercase_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_Shift_R, shift_r_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a));
}
} // namespace
