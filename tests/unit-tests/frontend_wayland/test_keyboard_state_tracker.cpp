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

#include <mir/test/doubles/advanceable_clock.h>
#include <mir/events/keyboard_event.h>
#include <mir/input/parameter_keymap.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{

/// Based on standard US QWERTY layout (PC keyboard / evdev scancodes)

constexpr uint32_t key_1_scancode = 2;
constexpr uint32_t key_0_scancode = 11;
constexpr uint32_t a_scancode = 30;
constexpr uint32_t b_scancode = 48;

constexpr uint32_t ctrl_l_scancode = 29;
constexpr uint32_t shift_l_scancode = 42;
constexpr uint32_t shift_r_scancode = 54;

class KeyboardStateTrackerTest : public Test
{
public:
    auto key_down(uint32_t keysym, uint32_t scancode, MirInputDeviceId id = device_id) -> mir::EventUPtr
    {
        auto& mod = modifier_states[id];
        if (keysym == XKB_KEY_Shift_L)
            mod |= mir_input_event_modifier_shift_left;
        if (keysym == XKB_KEY_Shift_R)
            mod |= mir_input_event_modifier_shift_right;

        auto event = mir::events::make_key_event(
            id,
            clock.now().time_since_epoch(),
            mir_keyboard_action_down,
            keysym,
            scancode,
            mod);
        event->to_input()->to_keyboard()->set_keymap(default_keymap);
        return event;
    }

    auto key_up(uint32_t keysym, uint32_t scancode, MirInputDeviceId id = device_id) -> mir::EventUPtr
    {
        auto& mod = modifier_states[id];
        if (keysym == XKB_KEY_Shift_L)
            mod &= ~mir_input_event_modifier_shift_left;
        if (keysym == XKB_KEY_Shift_R)
            mod &= ~mir_input_event_modifier_shift_right;

        auto event = mir::events::make_key_event(
            id,
            clock.now().time_since_epoch(),
            mir_keyboard_action_up,
            keysym,
            scancode,
            mod);
        event->to_input()->to_keyboard()->set_keymap(default_keymap);
        return event;
    }

    auto static inline const device_id = MirInputDeviceId{0};
    auto static inline const other_device_id = MirInputDeviceId{1};

    mtd::AdvanceableClock clock;
    mf::KeyboardStateTracker tracker;

    // Default US QWERTY keymap attached to every test event so that the
    // tracker's xkb_state is populated and shift-transition re-derivation
    // via xkb_state_key_get_one_sym() works correctly.
    std::shared_ptr<mir::input::Keymap> const default_keymap{
        std::make_shared<mir::input::ParameterKeymap>()};

    // Track modifier state per device to emulate Mir's tracking of modifiers
    std::unordered_map<MirInputDeviceId, MirInputEventModifiers> modifier_states;
};

TEST_F(KeyboardStateTrackerTest, initially_no_keys_pressed)
{
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.scancode_is_pressed(device_id, a_scancode));
}

TEST_F(KeyboardStateTrackerTest, tracks_key_down)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_TRUE(tracker.scancode_is_pressed(device_id, a_scancode));
}

TEST_F(KeyboardStateTrackerTest, tracks_key_up)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));
    tracker.process(*key_up(XKB_KEY_a, a_scancode));

    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.scancode_is_pressed(device_id, a_scancode));
}

TEST_F(KeyboardStateTrackerTest, tracks_multiple_keys)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));
    tracker.process(*key_down(XKB_KEY_b, b_scancode));
    tracker.process(*key_down(XKB_KEY_Control_L, ctrl_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_b));
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_Control_L));
    EXPECT_TRUE(tracker.scancode_is_pressed(device_id, a_scancode));
    EXPECT_TRUE(tracker.scancode_is_pressed(device_id, b_scancode));
    EXPECT_TRUE(tracker.scancode_is_pressed(device_id, ctrl_l_scancode));
}

TEST_F(KeyboardStateTrackerTest, lowercase_keysym_does_not_match_uppercase)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));

    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
}

TEST_F(KeyboardStateTrackerTest, uppercase_keysym_does_not_match_lowercase)
{
    tracker.process(*key_down(XKB_KEY_A, a_scancode));

    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, pressing_shift_l_promotes_held_lowercase_keysyms_to_uppercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, pressing_shift_l_after_lowercase_key_promotes_held_lowercase_keysym_to_uppercase)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));

    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
}
TEST_F(KeyboardStateTrackerTest, pressing_shift_r_promotes_held_lowercase_keysyms_to_uppercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_R, shift_r_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, releasing_shift_l_demotes_held_uppercase_keysyms_to_lowercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
}

TEST_F(KeyboardStateTrackerTest, releasing_shift_r_demotes_held_uppercase_keysyms_to_lowercase)
{
    tracker.process(*key_down(XKB_KEY_Shift_R, shift_r_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_R, shift_r_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
}

TEST_F(KeyboardStateTrackerTest, shift_promotion_does_not_affect_non_alpha_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_Control_L, ctrl_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_Control_L));
}

TEST_F(KeyboardStateTrackerTest, shift_demotion_does_not_affect_non_alpha_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_Control_L, ctrl_l_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_Control_L));
}

TEST_F(KeyboardStateTrackerTest, shift_promotion_affects_all_held_lowercase_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_down(XKB_KEY_B, b_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_B));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_b));
}

TEST_F(KeyboardStateTrackerTest, shift_demotion_affects_all_held_uppercase_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_down(XKB_KEY_B, b_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_b));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_B));
}

TEST_F(KeyboardStateTrackerTest, releasing_one_shift_when_both_are_pressed_does_not_demote_uppercase_keysyms)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_Shift_R, shift_r_scancode));
    tracker.process(*key_down(XKB_KEY_A, a_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
}
TEST_F(KeyboardStateTrackerTest, unknown_device_keysym_is_not_pressed)
{
    // Devices are not tracked until at least one input event from them is received.
    EXPECT_FALSE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, unknown_device_scancode_is_not_pressed)
{
    EXPECT_FALSE(tracker.scancode_is_pressed(other_device_id, a_scancode));
}

TEST_F(KeyboardStateTrackerTest, keys_on_different_devices_are_tracked_independently)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode, device_id));
    tracker.process(*key_down(XKB_KEY_b, b_scancode, other_device_id));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_b));

    EXPECT_FALSE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_a));
    EXPECT_TRUE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_b));
}

TEST_F(KeyboardStateTrackerTest, scancodes_on_different_devices_are_tracked_independently)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode, device_id));
    tracker.process(*key_down(XKB_KEY_b, b_scancode, other_device_id));

    EXPECT_TRUE(tracker.scancode_is_pressed(device_id, a_scancode));
    EXPECT_FALSE(tracker.scancode_is_pressed(device_id, b_scancode));

    EXPECT_FALSE(tracker.scancode_is_pressed(other_device_id, a_scancode));
    EXPECT_TRUE(tracker.scancode_is_pressed(other_device_id, b_scancode));
}

TEST_F(KeyboardStateTrackerTest, key_release_on_one_device_does_not_affect_other_device)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode, device_id));
    tracker.process(*key_down(XKB_KEY_a, a_scancode, other_device_id));
    tracker.process(*key_up(XKB_KEY_a, a_scancode, device_id));

    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a));
    EXPECT_TRUE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, shift_on_one_device_does_not_promote_keysyms_on_other_device)
{
    tracker.process(*key_down(XKB_KEY_a, a_scancode, other_device_id));
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode, device_id));

    // keysym on other_device_id should remain lowercase
    EXPECT_TRUE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_a));
    EXPECT_FALSE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_A));
}

TEST_F(KeyboardStateTrackerTest, shift_release_on_one_device_does_not_demote_keysyms_on_other_device)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode, device_id));
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode, other_device_id));
    tracker.process(*key_down(XKB_KEY_A, a_scancode, other_device_id));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode, device_id));

    // Shift was released only on device_id; other_device_id still has shift held
    EXPECT_TRUE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_A));
    EXPECT_FALSE(tracker.keysym_is_pressed(other_device_id, XKB_KEY_a));
}

TEST_F(KeyboardStateTrackerTest, pressing_shift_after_digit_promotes_to_symbol)
{
   tracker.process(*key_down(XKB_KEY_0, key_0_scancode));
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_0));

    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_parenright));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_0));
}

TEST_F(KeyboardStateTrackerTest, releasing_shift_after_digit_demotes_symbol_back_to_digit)
{
    // Shift held, '0' pressed (which the layout reports as ')'), then Shift
    // released: the tracker should revert to '0'.
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_parenright, key_0_scancode));
    tracker.process(*key_up(XKB_KEY_Shift_L, shift_l_scancode));

    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_0));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_parenright));
}

TEST_F(KeyboardStateTrackerTest, key_up_clears_key_when_modifier_changed_while_held)
{
    // Simulate: '1' pressed (keysym = XKB_KEY_1), then Shift pressed, then '1'
    // released while Shift is held. The key-up event reports XKB_KEY_exclam
    // ('!') because Shift is active. The tracker must still clear the pressed
    // state using the stored scancode rather than the key-up keysym.
    tracker.process(*key_down(XKB_KEY_1, key_1_scancode));
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_1));
    EXPECT_TRUE(tracker.scancode_is_pressed(device_id, key_1_scancode));

    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));

    // With a real xkb_state, pressing Shift re-derives '1' -> '!' correctly.
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_exclam));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_1));
    EXPECT_TRUE(tracker.scancode_is_pressed(device_id, key_1_scancode));

    // Key-up event reports XKB_KEY_exclam because Shift is still held.
    // The tracker must clear the entry by scancode, not by keysym.
    tracker.process(*key_up(XKB_KEY_exclam, key_1_scancode));

    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_1));
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_exclam));
    EXPECT_FALSE(tracker.scancode_is_pressed(device_id, key_1_scancode));
}
} // namespace
