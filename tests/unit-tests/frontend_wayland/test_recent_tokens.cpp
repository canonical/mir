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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;

using namespace testing;

namespace
{

TEST(RecentTokensTest, contains_added_item)
{
    mf::RecentTokens tokens;

    tokens.add("token42");

    EXPECT_TRUE(tokens.contains("token42"));
}

TEST(RecentTokensTest, maintains_multiple_items)
{
    mf::RecentTokens tokens;

    tokens.add("token1");
    tokens.add("token2");
    tokens.add("token3");

    EXPECT_TRUE(tokens.contains("token1"));
    EXPECT_TRUE(tokens.contains("token2"));
    EXPECT_TRUE(tokens.contains("token3"));
}

TEST(RecentTokensTest, evicts_oldest_when_exceeding_max_size)
{
    mf::RecentTokens tokens;

    // Add 33 tokens to exceed the capacity of 32
    for (int i = 0; i <= mf::RecentTokens::capacity; ++i)
    {
        tokens.add("token" + std::to_string(i));
    }

    EXPECT_FALSE(tokens.contains("token0"));
    EXPECT_TRUE(tokens.contains("token1"));
    EXPECT_TRUE(tokens.contains("token31"));
    EXPECT_TRUE(tokens.contains("token32"));
}

TEST(RecentTokensTest, evicts_in_fifo_order)
{
    mf::RecentTokens tokens;

    // Add 34 tokens to test FIFO eviction
    for (int i = 0; i <= mf::RecentTokens::capacity + 1; ++i)
    {
        tokens.add("token" + std::to_string(i));
    }

    EXPECT_FALSE(tokens.contains("token1"));
    EXPECT_FALSE(tokens.contains("token1"));
    EXPECT_TRUE(tokens.contains("token32"));
    EXPECT_TRUE(tokens.contains("token33"));
}

TEST(RecentTokensTest, allows_duplicate_items)
{
    mf::RecentTokens tokens;

    tokens.add("token42");
    tokens.add("token42");

    EXPECT_TRUE(tokens.contains("token42"));
}

} // namespace
