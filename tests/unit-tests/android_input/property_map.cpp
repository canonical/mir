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

#include "androidfw/Platform.h"

#include ANDROIDFW_UTILS(PropertyMap.h)

#include <fstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace android;

namespace
{
static char const* test_file = "AndroidInputPropertyMap.ini";

struct AndroidInputPropertyMap : public ::testing::Test
{
    PropertyMap* test_map;

    static void SetUpTestCase()
    {
        ASSERT_TRUE(std::ofstream(test_file) <<
            "test_string=a_string\n"
            "#test_bool_true=true\n"
            "test_bool_true=1\n"
            "test_bool_false=0\n"
            "#test_bool_true=true\n"
            "# a comment\n"
            "test_int_32=123\n");
    }

    void SetUp()
    {
        ASSERT_EQ(NO_ERROR, PropertyMap::load(String8(test_file), &test_map));
    }

    void TearDown()
    {
        delete test_map;
    }
};
}

TEST_F(AndroidInputPropertyMap, test_map_created)
{
    EXPECT_TRUE(test_map);
}

TEST_F(AndroidInputPropertyMap, test_map_has_a_string)
{
    String8 result;
    EXPECT_TRUE(test_map->tryGetProperty(String8("test_string"), result));
    EXPECT_EQ(String8("a_string"), result);
}

TEST_F(AndroidInputPropertyMap, test_map_has_an_int32_t)
{
    int32_t result;
    EXPECT_TRUE(test_map->tryGetProperty(String8("test_int_32"), result));
    EXPECT_EQ(123, result);
}

TEST_F(AndroidInputPropertyMap, test_map_has_bools_true_and_false)
{
    bool result{};
    EXPECT_TRUE(test_map->tryGetProperty(String8("test_bool_true"), result));
    EXPECT_TRUE(result);

    EXPECT_TRUE(test_map->tryGetProperty(String8("test_bool_false"), result));
    EXPECT_FALSE(result);
}
