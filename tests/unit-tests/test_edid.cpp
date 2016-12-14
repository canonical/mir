/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/edid.h"
#include <gtest/gtest.h>

using mir::graphics::Edid;

namespace
{
unsigned char const dell_u2413_edid[] =
    "\x00\xff\xff\xff\xff\xff\xff\x00\x10\xac\x46\xf0\x4c\x4a\x31\x41"
    "\x05\x19\x01\x04\xb5\x34\x20\x78\x3a\x1d\xf5\xae\x4f\x35\xb3\x25"
    "\x0d\x50\x54\xa5\x4b\x00\x81\x80\xa9\x40\xd1\x00\x71\x4f\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x28\x3c\x80\xa0\x70\xb0\x23\x40\x30\x20"
    "\x36\x00\x06\x44\x21\x00\x00\x1a\x00\x00\x00\xff\x00\x59\x43\x4d"
    "\x30\x46\x35\x31\x52\x41\x31\x4a\x4c\x0a\x00\x00\x00\xfc\x00\x44"
    "\x45\x4c\x4c\x20\x55\x32\x34\x31\x33\x0a\x20\x20\x00\x00\x00\xfd"
    "\x00\x38\x4c\x1e\x51\x11\x00\x0a\x20\x20\x20\x20\x20\x20\x01\x42"
    "\x02\x03\x1d\xf1\x50\x90\x05\x04\x03\x02\x07\x16\x01\x1f\x12\x13"
    "\x14\x20\x15\x11\x06\x23\x09\x1f\x07\x83\x01\x00\x00\x02\x3a\x80"
    "\x18\x71\x38\x2d\x40\x58\x2c\x45\x00\x06\x44\x21\x00\x00\x1e\x01"
    "\x1d\x80\x18\x71\x1c\x16\x20\x58\x2c\x25\x00\x06\x44\x21\x00\x00"
    "\x9e\x01\x1d\x00\x72\x51\xd0\x1e\x20\x6e\x28\x55\x00\x06\x44\x21"
    "\x00\x00\x1e\x8c\x0a\xd0\x8a\x20\xe0\x2d\x10\x10\x3e\x96\x00\x06"
    "\x44\x21\x00\x00\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x09";
} // namespace

TEST(EDID, has_correct_size)
{
    EXPECT_EQ(128u, sizeof(Edid));
    EXPECT_EQ(128u, Edid::minimum_size);
}

TEST(EDID, can_get_name)
{
    auto edid = reinterpret_cast<Edid const*>(dell_u2413_edid);
    Edid::MonitorName name;
    int len = edid->get_monitor_name(name);
    EXPECT_EQ(10, len);
    EXPECT_STREQ("DELL U2413", name);
}

TEST(EDID, can_get_manufacturer)
{
    auto edid = reinterpret_cast<Edid const*>(dell_u2413_edid);
    Edid::Manufacturer man;
    int len = edid->get_manufacturer(man);
    EXPECT_EQ(3, len);
    EXPECT_STREQ("DEL", man);
}

TEST(EDID, can_get_product_code)
{
    auto edid = reinterpret_cast<Edid const*>(dell_u2413_edid);
    EXPECT_EQ(61510u, edid->product_code());
}
