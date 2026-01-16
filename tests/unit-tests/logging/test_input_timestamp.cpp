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
#include <mir/time/clock.h>
#include <chrono>
#include <string>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace testing;
namespace ml = mir::logging;

struct FakeClock : mir::time::Clock
{
    mir::time::Timestamp current_time_value{};

    mir::time::Timestamp now() const override
    {
        return current_time_value;
    }

    mir::time::Duration min_wait_until(mir::time::Timestamp) const override
    {
        return mir::time::Duration::zero();
    }
};

TEST(TimestampTest, past_time_is_correctly_formatted)
{
    FakeClock clock;
    clock.current_time_value = steady_clock::time_point(1000ms);
    auto const past_event = 250ms;

    std::string out = ml::input_timestamp(clock, past_event);

    EXPECT_THAT(out, AllOf(HasSubstr("750"), HasSubstr("ms ago")));
}

TEST(TimestampTest, future_time_is_correctly_formatted)
{
    FakeClock clock;
    clock.current_time_value = steady_clock::time_point(500ms);
    auto const future_event = 750ms;

    std::string out = ml::input_timestamp(clock, future_event);

    EXPECT_THAT(out, AllOf(HasSubstr("250"), HasSubstr("ms"), HasSubstr("in the future")));
}
