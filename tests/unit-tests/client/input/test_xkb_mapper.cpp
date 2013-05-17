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

#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>

#include <gtest/gtest.h>

namespace mircv = mir::input::receiver;

namespace
{
static MirKeyEvent key_press_event(int scan_code)
{
    MirKeyEvent ev;
    ev.action = mir_key_action_down;
    ev.scan_code = scan_code;
    return ev;
}
static MirKeyEvent key_release_event(int scan_code)
{
    MirKeyEvent ev;
    ev.action = mir_key_action_up;
    ev.scan_code = scan_code;
    return ev;
}
}

TEST(XKBMapper, maps_generic_us_english_keys)
{
    mircv::XKBMapper mapper;

    // TODO: Cleanup ~racarr
    
    auto ev = key_press_event(KEY_4);
    mapper.update_state_and_map_event(ev);
    EXPECT_EQ(XKB_KEY_4, ev.key_code);

    ev = key_press_event(KEY_LEFTSHIFT);
    mapper.update_state_and_map_event(ev);
    EXPECT_EQ(XKB_KEY_Shift_L, ev.key_code);

    ev = key_press_event(KEY_4);
    mapper.update_state_and_map_event(ev);
    EXPECT_EQ(XKB_KEY_dollar, ev.key_code);

    ev = key_release_event(KEY_4);
    mapper.update_state_and_map_event(ev);
    EXPECT_EQ(XKB_KEY_dollar, ev.key_code);

    ev = key_release_event(KEY_LEFTSHIFT);
    mapper.update_state_and_map_event(ev);
    EXPECT_EQ(XKB_KEY_Shift_L, ev.key_code);

    ev = key_press_event(KEY_4);
    mapper.update_state_and_map_event(ev);
    EXPECT_EQ(XKB_KEY_4, ev.key_code);
}
