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
#include "src/server/frontend_wayland/input_trigger_registration_v1.h"

#include <mir/test/doubles/advanceable_clock.h>
#include <mir/events/keyboard_event.h>
#include <mir/input/parameter_keymap.h>
#include <mir/test/doubles/mock_device.h>
#include <mir/test/doubles/mock_input_device_hub.h>

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
constexpr uint32_t s_scancode = 31;

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

struct ShiftedKeysym
{
    xkb_keysym_t keysym_to_match;
    xkb_keysym_t shifted_keysym;
    uint32_t scancode;
};

class KeyboardStateTrackerShiftTest :
    public KeyboardStateTrackerTest, public WithParamInterface<ShiftedKeysym>
{
};

TEST_P(KeyboardStateTrackerShiftTest, matches_shifted_and_unshifted_symbols_when_unshifted_matching_is_enabled)
{
    auto const& key = GetParam();
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(key.shifted_keysym, key.scancode));

    auto constexpr include_unshifted = true;
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, key.keysym_to_match, include_unshifted));
}

INSTANTIATE_TEST_SUITE_P(
    KeyboardStateTrackerShiftCases,
    KeyboardStateTrackerShiftTest,
    Values(
        ShiftedKeysym{XKB_KEY_s, XKB_KEY_S, s_scancode},
        ShiftedKeysym{XKB_KEY_S, XKB_KEY_S, s_scancode},
        ShiftedKeysym{XKB_KEY_1, XKB_KEY_exclam, key_1_scancode},
        ShiftedKeysym{XKB_KEY_exclam, XKB_KEY_exclam, key_1_scancode}));

TEST_F(KeyboardStateTrackerTest, unshifted_matching_does_not_match_an_unrelated_symbol)
{
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));
    tracker.process(*key_down(XKB_KEY_S, s_scancode));

    auto constexpr include_unshifted = true;
    EXPECT_FALSE(tracker.keysym_is_pressed(device_id, XKB_KEY_a, include_unshifted));
}

TEST_F(KeyboardStateTrackerTest, matches_unshifted_symbol_when_shift_is_pressed_after_letter)
{
    tracker.process(*key_down(XKB_KEY_s, s_scancode));
    tracker.process(*key_down(XKB_KEY_Shift_L, shift_l_scancode));

    auto constexpr include_unshifted = true;
    EXPECT_TRUE(tracker.keysym_is_pressed(device_id, XKB_KEY_s, include_unshifted));
}

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

namespace
{
using PM = mir::wayland::InputTriggerRegistrationManagerV1::Modifiers;
using ShortcutKey = mf::KeyboardStateTracker::ShortcutKey;

auto sym(uint32_t value) -> ShortcutKey { return {ShortcutKey::Type::keysym, value}; }
auto code(uint32_t value) -> ShortcutKey { return {ShortcutKey::Type::scancode, value}; }

class MockShortcutTrigger : public mf::InputTriggerRegistry::Trigger
{
public:
    MOCK_METHOD(bool, is_same_trigger, (Trigger const*), (const, override));
    MOCK_METHOD(bool, overlaps, (Trigger const*), (const, override));
    MOCK_METHOD(bool, is_active, (), (const, override));
    MOCK_METHOD(EventOutcome, check_event, (MirEvent const&), (override));
};

struct ShortcutOverlapCase
{
    uint32_t first_modifiers;
    ShortcutKey first;
    uint32_t second_modifiers;
    ShortcutKey second;
    char const* layout;
    bool overlaps;
};

class KeyboardShortcutOverlap : public TestWithParam<ShortcutOverlapCase>
{
public:
    KeyboardShortcutOverlap()
    {
        ON_CALL(*hub, for_each_input_device(_)).WillByDefault(Invoke(
            [this](auto const& callback) { callback(device); }));
    }

    auto modifiers(uint32_t protocol, ShortcutKey key) -> mf::InputTriggerModifiers
    {
        return mf::InputTriggerModifiers::from_protocol(
            protocol, key.type == ShortcutKey::Type::keysym && key.value >= XKB_KEY_A && key.value <= XKB_KEY_Z);
    }

    std::shared_ptr<NiceMock<mtd::MockInputDeviceHub>> hub{
        std::make_shared<NiceMock<mtd::MockInputDeviceHub>>()};
    NiceMock<mtd::MockDevice> device{0, mir::input::DeviceCapability::keyboard, "keyboard", "keyboard"};
    mf::KeyboardStateTracker tracker{hub};
};

TEST_P(KeyboardShortcutOverlap, checks_configured_keymap_before_any_key_event)
{
    auto const& test = GetParam();
    auto const keymap = std::make_shared<mir::input::ParameterKeymap>("pc105", test.layout, "", "");
    ON_CALL(device, keyboard_configuration()).WillByDefault(Return(MirKeyboardConfig{keymap}));

    auto const first_modifiers = modifiers(test.first_modifiers, test.first);
    auto const second_modifiers = modifiers(test.second_modifiers, test.second);
    EXPECT_EQ(test.overlaps, tracker.shortcuts_overlap(first_modifiers, test.first, second_modifiers, test.second));
    EXPECT_EQ(test.overlaps, tracker.shortcuts_overlap(second_modifiers, test.second, first_modifiers, test.first));

    mf::InputTriggerRegistry registry;
    NiceMock<MockShortcutTrigger> accepted;
    NiceMock<MockShortcutTrigger> requested;
    ASSERT_TRUE(registry.register_trigger(&accepted));
    EXPECT_CALL(requested, overlaps(&accepted)).WillOnce(Invoke(
        [&](auto) { return tracker.shortcuts_overlap(first_modifiers, test.first, second_modifiers, test.second); }));
    EXPECT_EQ(!test.overlaps, registry.register_trigger(&requested));
}

INSTANTIATE_TEST_SUITE_P(ConfiguredLayouts, KeyboardShortcutOverlap, Values(
    ShortcutOverlapCase{PM::ctrl, sym(XKB_KEY_s), PM::ctrl, sym(XKB_KEY_s), "us", true},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_s), PM::shift, sym(XKB_KEY_S), "us", true},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_1), PM::shift, sym(XKB_KEY_exclam), "us", true},
    ShortcutOverlapCase{PM::ctrl, sym(XKB_KEY_S), PM::ctrl | PM::shift, sym(XKB_KEY_s), "us", true},
    ShortcutOverlapCase{PM::ctrl, sym(XKB_KEY_s), PM::ctrl, sym(XKB_KEY_S), "us", false},
    ShortcutOverlapCase{PM::ctrl, sym(XKB_KEY_s), PM::ctrl | PM::shift, sym(XKB_KEY_s), "us", false},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_s), PM::shift_left, sym(XKB_KEY_s), "us", true},
    ShortcutOverlapCase{PM::shift_left, sym(XKB_KEY_s), PM::shift_right, sym(XKB_KEY_s), "us", false},
    ShortcutOverlapCase{PM::shift_left, sym(XKB_KEY_S), PM::shift_right, sym(XKB_KEY_S), "us", true},
    ShortcutOverlapCase{PM::shift_left, sym(XKB_KEY_s), PM::shift_right, sym(XKB_KEY_S), "us", false},
    ShortcutOverlapCase{PM::ctrl_left, sym(XKB_KEY_s), PM::ctrl_right, sym(XKB_KEY_s), "us", false},
    ShortcutOverlapCase{PM::ctrl, sym(XKB_KEY_s), PM::alt, sym(XKB_KEY_s), "us", false},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_s), PM::shift, sym(XKB_KEY_a), "us", false},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_1), PM::shift, sym(XKB_KEY_exclam), "fr", false},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_1), PM::shift, sym(XKB_KEY_ampersand), "fr", true},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_ampersand), PM::shift, sym(XKB_KEY_exclamdown), "fr", true},
    ShortcutOverlapCase{PM::shift, sym(XKB_KEY_1), PM::shift, sym(XKB_KEY_exclam), "fr,us", true},
    ShortcutOverlapCase{PM::ctrl, code(s_scancode), PM::ctrl_left, code(s_scancode), "us", true},
    ShortcutOverlapCase{PM::ctrl | PM::function, code(s_scancode),
        PM::ctrl_left | PM::function, code(s_scancode), "us", true},
    ShortcutOverlapCase{PM::ctrl, code(s_scancode), PM::ctrl, code(a_scancode), "us", false},
    ShortcutOverlapCase{PM::shift, code(s_scancode), PM::shift, sym(XKB_KEY_S), "us", true},
    ShortcutOverlapCase{PM::shift, code(key_1_scancode), PM::shift, sym(XKB_KEY_exclam), "us", true},
    ShortcutOverlapCase{PM::shift, code(key_1_scancode), PM::shift, sym(XKB_KEY_exclam), "fr", false},
    ShortcutOverlapCase{PM::ctrl, code(s_scancode), PM::ctrl | PM::shift, sym(XKB_KEY_S), "us", false}));

TEST_F(KeyboardShortcutOverlap, reads_changes_to_configured_keymaps)
{
    auto keymap = std::make_shared<mir::input::ParameterKeymap>("pc105", "fr", "", "");
    ON_CALL(device, keyboard_configuration()).WillByDefault(Invoke(
        [&keymap] { return MirKeyboardConfig{keymap}; }));
    auto const shift = mf::InputTriggerModifiers::from_protocol(PM::shift);
    EXPECT_FALSE(tracker.shortcuts_overlap(shift, sym(XKB_KEY_1), shift, sym(XKB_KEY_exclam)));
    keymap = std::make_shared<mir::input::ParameterKeymap>("pc105", "us", "", "");
    EXPECT_TRUE(tracker.shortcuts_overlap(shift, sym(XKB_KEY_1), shift, sym(XKB_KEY_exclam)));
}

TEST_F(KeyboardShortcutOverlap, checks_each_keyboard)
{
    auto const french = std::make_shared<mir::input::ParameterKeymap>("pc105", "fr", "", "");
    auto const us = std::make_shared<mir::input::ParameterKeymap>("pc105", "us", "", "");
    NiceMock<mtd::MockDevice> other{1, mir::input::DeviceCapability::keyboard, "other", "other"};
    ON_CALL(device, keyboard_configuration()).WillByDefault(Return(MirKeyboardConfig{french}));
    ON_CALL(other, keyboard_configuration()).WillByDefault(Return(MirKeyboardConfig{us}));
    ON_CALL(*hub, for_each_input_device(_)).WillByDefault(Invoke(
        [&](auto const& callback) { callback(device); callback(other); }));
    auto const shift = mf::InputTriggerModifiers::from_protocol(PM::shift);
    EXPECT_TRUE(tracker.shortcuts_overlap(shift, sym(XKB_KEY_1), shift, sym(XKB_KEY_exclam)));
}

TEST_F(KeyboardShortcutOverlap, accounts_for_symbols_retained_after_other_modifier_changes)
{
    auto const keymap = std::make_shared<mir::input::ParameterKeymap>("pc105", "fr", "", "");
    ON_CALL(device, keyboard_configuration()).WillByDefault(Return(MirKeyboardConfig{keymap}));
    mtd::AdvanceableClock clock;
    auto const shift = mir_input_event_modifier_shift | mir_input_event_modifier_shift_left;
    auto const altgr = mir_input_event_modifier_alt | mir_input_event_modifier_alt_right;
    auto const right_alt_scancode = 100u;
    auto process = [&](MirKeyboardAction action, uint32_t keysym, uint32_t scancode, MirInputEventModifiers mods)
    {
        auto event = mir::events::make_key_event(0, clock.now().time_since_epoch(), action, keysym, scancode, mods);
        event->to_input()->to_keyboard()->set_keymap(keymap);
        tracker.process(*event);
    };
    process(mir_keyboard_action_down, XKB_KEY_Shift_L, shift_l_scancode, shift);
    process(mir_keyboard_action_down, XKB_KEY_ISO_Level3_Shift, right_alt_scancode, shift | altgr);
    process(mir_keyboard_action_down, XKB_KEY_exclamdown, key_1_scancode, shift | altgr);
    process(mir_keyboard_action_up, XKB_KEY_ISO_Level3_Shift, right_alt_scancode, shift);

    EXPECT_TRUE(tracker.keysym_is_pressed(0, XKB_KEY_ampersand, true));
    EXPECT_TRUE(tracker.keysym_is_pressed(0, XKB_KEY_exclamdown, true));
    auto const required_shift = mf::InputTriggerModifiers::from_protocol(PM::shift);
    EXPECT_TRUE(tracker.shortcuts_overlap(
        required_shift, sym(XKB_KEY_ampersand), required_shift, sym(XKB_KEY_exclamdown)));
}

TEST(InputTriggerRegistryOverlap, default_predicate_preserves_exact_identity)
{
    mf::InputTriggerRegistry registry;
    NiceMock<MockShortcutTrigger> accepted;
    NiceMock<MockShortcutTrigger> requested;
    ASSERT_TRUE(registry.register_trigger(&accepted));
    EXPECT_CALL(requested, overlaps(&accepted)).WillOnce(Invoke(
        [&](auto other) { return requested.mf::InputTriggerRegistry::Trigger::overlaps(other); }));
    EXPECT_CALL(requested, is_same_trigger(&accepted)).WillOnce(Return(true));
    EXPECT_FALSE(registry.register_trigger(&requested));
}
}
