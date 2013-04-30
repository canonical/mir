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

namespace mcli = mir::client::input;

TEST(XKBMapper, maps_generic_us_english_keys)
{
    mcli::XKBMapper mapper;

    EXPECT_EQ(static_cast<xkb_keysym_t>(XKB_KEY_4), mapper.press_and_map_key(KEY_4));
    EXPECT_EQ(static_cast<xkb_keysym_t>(XKB_KEY_Shift_L), mapper.press_and_map_key(KEY_LEFTSHIFT));
    EXPECT_EQ(static_cast<xkb_keysym_t>(XKB_KEY_dollar), mapper.press_and_map_key(KEY_4));
    EXPECT_EQ(static_cast<xkb_keysym_t>(XKB_KEY_dollar), mapper.release_and_map_key(KEY_4));
    EXPECT_EQ(static_cast<xkb_keysym_t>(XKB_KEY_Shift_L), mapper.release_and_map_key(KEY_LEFTSHIFT));
    EXPECT_EQ(static_cast<xkb_keysym_t>(XKB_KEY_4), mapper.press_and_map_key(KEY_4));
}
