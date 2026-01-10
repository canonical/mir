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

TEST(TimestampTest, past_time_is_correctly_formatted)
{
    auto const past_event = steady_clock::now().time_since_epoch() - 500ms;

    std::string out = ml::input_timestamp(past_event);

    EXPECT_THAT(out, Not(HasSubstr(".-")));
    EXPECT_THAT(out, ContainsRegex("5[0-9][0-9]\\."));
    EXPECT_THAT(out, HasSubstr("ms ago"));
}

TEST(TimestampTest, future_time_is_correctly_formatted)
{
    auto const future_event = steady_clock::now().time_since_epoch() + 1000ms + 10us;

    std::string out = ml::input_timestamp(future_event);

    EXPECT_THAT(out, Not(HasSubstr(".-")));
    EXPECT_THAT(out, ContainsRegex("(9[0-9][0-9]|1000)\\."));
    EXPECT_THAT(out, HasSubstr("ms"));
    EXPECT_THAT(out, HasSubstr("in the future"));

}
