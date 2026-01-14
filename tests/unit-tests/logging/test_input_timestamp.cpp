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

#include <mir/logging/input_timestamp.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <string>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace testing;
namespace ml = mir::logging;

struct FakeClock
{
    using time_point = steady_clock::time_point;

    time_point now() const
    {
        return current_time;
    }

    time_point current_time{};
};

TEST(TimestampTest, past_time_is_correctly_formatted)
{
    FakeClock clock;
    clock.current_time = steady_clock::time_point(1000ms);
    auto const past_event = 250ms;

    std::string out = ml::input_timestamp(clock, past_event);

    EXPECT_THAT(out, HasSubstr("750"));
    EXPECT_THAT(out, HasSubstr("ms ago"));
}

TEST(TimestampTest, future_time_is_correctly_formatted)
{
    FakeClock clock;
    clock.current_time = steady_clock::time_point(500ms);
    auto const future_event = 750ms;

    std::string out = ml::input_timestamp(clock, future_event);

    EXPECT_THAT(out, HasSubstr("250"));
    EXPECT_THAT(out, HasSubstr("ms"));
    EXPECT_THAT(out, HasSubstr("in the future"));
}
