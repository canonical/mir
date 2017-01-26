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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/time/posix_timestamp.h"
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace mir::time;

TEST(PosixTimestamp, equal)
{
    EXPECT_TRUE(PosixTimestamp(CLOCK_MONOTONIC,123ns) ==
                PosixTimestamp(CLOCK_MONOTONIC,123ns));
    EXPECT_FALSE(PosixTimestamp(CLOCK_MONOTONIC,123ns) ==
                 PosixTimestamp(CLOCK_MONOTONIC,456ns));
    EXPECT_FALSE(PosixTimestamp(CLOCK_MONOTONIC,123ns) ==
                 PosixTimestamp(CLOCK_REALTIME,123ns));
    EXPECT_FALSE(PosixTimestamp(CLOCK_MONOTONIC,0ns) ==
                 PosixTimestamp(CLOCK_REALTIME,0ns));
}

TEST(PosixTimestamp, subtract)
{
    EXPECT_EQ(PosixTimestamp(CLOCK_MONOTONIC,333ns),
              PosixTimestamp(CLOCK_MONOTONIC,789ns) - 456ns);
    EXPECT_EQ(PosixTimestamp(CLOCK_REALTIME,0ns),
              PosixTimestamp(CLOCK_REALTIME,123ns) - 123ns);

    EXPECT_EQ(456ns,
              PosixTimestamp(CLOCK_MONOTONIC,789ns) -
              PosixTimestamp(CLOCK_MONOTONIC,333ns));
    EXPECT_THROW(
              PosixTimestamp(CLOCK_MONOTONIC,789ns) -
              PosixTimestamp(CLOCK_REALTIME,333ns),
              std::logic_error);
}

TEST(PosixTimestamp, add)
{
    EXPECT_EQ(PosixTimestamp(CLOCK_MONOTONIC,579ns),
              PosixTimestamp(CLOCK_MONOTONIC,456ns) + 123ns);
    EXPECT_EQ(PosixTimestamp(CLOCK_REALTIME,0ns),
              PosixTimestamp(CLOCK_REALTIME,123ns) + -123ns);
}

TEST(PosixTimestamp, modulo)
{
    EXPECT_EQ(654321ns,
              PosixTimestamp(CLOCK_MONOTONIC,987654321ns) % 1000000ns);
}

TEST(PosixTimestamp, comparison)
{
    PosixTimestamp const a(CLOCK_MONOTONIC, 111ns);
    PosixTimestamp const aa(CLOCK_REALTIME, 111ns);
    PosixTimestamp const b(CLOCK_MONOTONIC, 333ns);
    PosixTimestamp const bb(CLOCK_REALTIME, 333ns);

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(aa < bb);
    EXPECT_TRUE(bb > aa);

    EXPECT_THROW((void)(a < bb), std::logic_error);
    EXPECT_THROW((void)(a > bb), std::logic_error);
    EXPECT_THROW((void)(aa > b), std::logic_error);
    EXPECT_THROW((void)(aa < b), std::logic_error);

    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(b >= b);
    EXPECT_TRUE(aa <= bb);
    EXPECT_TRUE(aa <= aa);
    EXPECT_TRUE(bb >= aa);
    EXPECT_TRUE(bb >= bb);

    EXPECT_THROW((void)(a <= bb), std::logic_error);
    EXPECT_THROW((void)(a >= bb), std::logic_error);
    EXPECT_THROW((void)(aa >= b), std::logic_error);
    EXPECT_THROW((void)(aa <= b), std::logic_error);
}

TEST(PosixTimestamp, sleeps)
{
    auto const start = PosixTimestamp::now(CLOCK_MONOTONIC);
    auto const expected = 123456789ns;
    sleep_until(start + expected);
    auto const end = PosixTimestamp::now(CLOCK_MONOTONIC);
    auto const observed = end - start;
    EXPECT_LE(expected, observed);
    EXPECT_GT(100*expected, observed);
}

TEST(PosixTimestamp, wait_for_the_past_is_immediate)
{
    auto const start = PosixTimestamp::now(CLOCK_MONOTONIC);
    auto const the_past = start - 9876543210ns;
    sleep_until(the_past);
    auto const end = PosixTimestamp::now(CLOCK_MONOTONIC);
    auto const delay = end - start;
    EXPECT_GT(5s, delay);
}
