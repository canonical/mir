/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/xkb_mapper.h"
#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"

#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>

#include <gtest/gtest.h>

namespace mircv = mir::input::receiver;
namespace mev = mir::events;

namespace
{

static int map_key(mircv::XKBMapper &mapper, MirKeyboardAction action, int scan_code)
{
    auto mac = 0;
    auto ev = mev::make_event(MirInputDeviceId(0), std::chrono::nanoseconds(0), mac, action,
                              0, scan_code, mir_input_event_modifier_none);

    mapper.update_state_and_map_event(*ev);
    return ev->key.key_code;
}

}

TEST(XKBMapper, maps_generic_us_english_keys)
{
    mircv::XKBMapper mapper;

    EXPECT_EQ(XKB_KEY_4, map_key(mapper, mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mapper, mir_keyboard_action_down, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mapper, mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mapper, mir_keyboard_action_up, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mapper, mir_keyboard_action_up, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_4, map_key(mapper, mir_keyboard_action_down, KEY_4));
}

TEST(XKBMapper, key_repeats_do_not_recurse_modifier_state)
{
    mircv::XKBMapper mapper;

    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mapper, mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mapper, mir_keyboard_action_repeat, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_ampersand, map_key(mapper, mir_keyboard_action_down, KEY_7));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mapper, mir_keyboard_action_up, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_7, map_key(mapper, mir_keyboard_action_down, KEY_7));
}
