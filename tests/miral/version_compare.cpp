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

#include "version_compare.h"

#include <gtest/gtest.h>

#include <string>

namespace mlc = miral::live_config;

using testing::Test;

namespace
{
// Convenience predicates expressing the strcmp-like contract of version_compare.
auto orders_before(std::string_view lhs, std::string_view rhs) -> bool
{
    return mlc::version_compare(lhs, rhs) < 0;
}

auto orders_equal(std::string_view lhs, std::string_view rhs) -> bool
{
    return mlc::version_compare(lhs, rhs) == 0;
}
}

TEST(VersionCompare, identical_strings_compare_equal)
{
    EXPECT_TRUE(orders_equal("10-first.conf", "10-first.conf"));
    EXPECT_TRUE(orders_equal("", ""));
}

TEST(VersionCompare, is_antisymmetric)
{
    EXPECT_TRUE(std::is_lt(mlc::version_compare("a", "b")));
    EXPECT_TRUE(std::is_gt(mlc::version_compare("b", "a")));
}

TEST(VersionCompare, non_digit_runs_compare_byte_wise)
{
    EXPECT_TRUE(orders_before("abc", "abd"));
    EXPECT_TRUE(orders_before("Zebra", "apple")); // 'Z' (0x5A) < 'a' (0x61)
}

TEST(VersionCompare, single_digit_runs_compare_numerically)
{
    EXPECT_TRUE(orders_before("2", "10"));
    EXPECT_TRUE(orders_before("9", "10"));
    EXPECT_TRUE(orders_before("99", "100"));
}

// The motivating case: drop-in config files with numeric prefixes must sort
// by numeric value of the prefix, not by raw byte order (where "10-" < "2-").
TEST(VersionCompare, numeric_prefixes_sort_numerically)
{
    EXPECT_TRUE(orders_before("2-a.conf", "10-b.conf"));
    EXPECT_TRUE(orders_before("1-c.conf", "2-a.conf"));
    EXPECT_TRUE(orders_before("9-z.conf", "10-a.conf"));
}

TEST(VersionCompare, embedded_digit_runs_compare_numerically)
{
    EXPECT_TRUE(orders_before("file2", "file10"));
    EXPECT_TRUE(orders_before("img-9.png", "img-10.png"));
}

TEST(VersionCompare, equal_numeric_value_breaks_tie_on_leading_zeros)
{
    // Same numeric value: fewer leading zeros sorts first, so the order is total
    // and distinct strings never compare equal.
    EXPECT_FALSE(orders_equal("1", "01"));
    EXPECT_TRUE(orders_before("1", "01"));
}

TEST(VersionCompare, zero_padded_prefixes_sort_numerically)
{
    EXPECT_TRUE(orders_before("020-x.conf", "100-x.conf"));
    EXPECT_TRUE(orders_before("09-x.conf", "10-x.conf"));
}
