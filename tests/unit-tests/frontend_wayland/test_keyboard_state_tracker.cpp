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

#include "src/server/frontend_wayland/input_trigger_registry.h"
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

constexpr uint32_t KEY_A_SCANCODE = 30;
constexpr uint32_t KEY_B_SCANCODE = 48;

constexpr uint32_t KEY_CTRL_L_SCANCODE = 37;

class KeyboardStateTrackerTest : public Test
{
public:
    auto key_down(uint32_t keysym, uint32_t scancode) -> mir::EventUPtr
    {
        return event_builder.key_event(std::nullopt, mir_keyboard_action_down, keysym, scancode);
    }

    auto key_up(uint32_t keysym, uint32_t scancode) -> mir::EventUPtr
    {
        return event_builder.key_event(std::nullopt, mir_keyboard_action_up, keysym, scancode);
    }

    mtd::AdvanceableClock clock;
    mir::input::DefaultEventBuilder event_builder{MirInputDeviceId{0}, mir::test::fake_shared(clock)};
    mf::KeyboardStateTracker tracker;
};

TEST_F(KeyboardStateTrackerTest, initially_no_keys_pressed)
{
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a, false));
    EXPECT_FALSE(tracker.scancode_is_pressed(KEY_A_SCANCODE));
}

TEST_F(KeyboardStateTrackerTest, tracks_key_down)
{
    tracker.process(*key_down(XKB_KEY_a, KEY_A_SCANCODE));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a, false));
    EXPECT_TRUE(tracker.scancode_is_pressed(KEY_A_SCANCODE));
}

TEST_F(KeyboardStateTrackerTest, tracks_key_up)
{
    tracker.process(*key_down(XKB_KEY_a, KEY_A_SCANCODE));
    tracker.process(*key_up(XKB_KEY_a, KEY_A_SCANCODE));

    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_a, false));
    EXPECT_FALSE(tracker.scancode_is_pressed(KEY_A_SCANCODE));
}

TEST_F(KeyboardStateTrackerTest, tracks_multiple_keys)
{
    tracker.process(*key_down(XKB_KEY_a, KEY_A_SCANCODE));
    tracker.process(*key_down(XKB_KEY_b, KEY_B_SCANCODE));
    tracker.process(*key_down(XKB_KEY_Control_L, KEY_CTRL_L_SCANCODE));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a, false));
    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_b, false));
    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_Control_L, false));
    EXPECT_TRUE(tracker.scancode_is_pressed(KEY_A_SCANCODE));
    EXPECT_TRUE(tracker.scancode_is_pressed(KEY_B_SCANCODE));
    EXPECT_TRUE(tracker.scancode_is_pressed(KEY_CTRL_L_SCANCODE));
}

TEST_F(KeyboardStateTrackerTest, case_sensitive_exact_match)
{
    tracker.process(*key_down(XKB_KEY_a, KEY_A_SCANCODE));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a, false));
    EXPECT_FALSE(tracker.keysym_is_pressed(XKB_KEY_A, false));
}

TEST_F(KeyboardStateTrackerTest, case_insensitive_lowercase_matches_uppercase)
{
    tracker.process(*key_down(XKB_KEY_a, KEY_A_SCANCODE));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_A, true));
}

TEST_F(KeyboardStateTrackerTest, case_insensitive_uppercase_matches_lowercase)
{
    tracker.process(*key_down(XKB_KEY_A, KEY_A_SCANCODE));

    EXPECT_TRUE(tracker.keysym_is_pressed(XKB_KEY_a, true));
}
} // namespace
