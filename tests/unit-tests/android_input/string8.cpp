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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#include <std/String8.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace android;

TEST(AndroidString8, appendFormat)
{
    String8 string;

    EXPECT_STREQ("", c_str(string));

    appendFormat(string, "%s", "Hello");

    EXPECT_STREQ("Hello", c_str(string));

    u_char digits = 0x12;

    appendFormat(string, "%02x", digits);
    EXPECT_STREQ("Hello12", c_str(string));
}

TEST(AndroidString8, formatString8)
{
    auto string = formatString8("Hello %s #%02x", "world", 0x42);

    EXPECT_STREQ("Hello world #42", c_str(string));
}
