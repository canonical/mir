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
#include <chrono>
#include <string>
#include <cstdio>

using namespace std::chrono;
namespace ml = mir::logging;

class TimestampTest : public ::testing::Test {
};

TEST_F(TimestampTest, HandlesPastTime) {
    auto now = steady_clock::now().time_since_epoch();
    nanoseconds past_event = duration_cast<nanoseconds>(now - milliseconds(500));

    std::string out = ml::input_timestamp(past_event);

    EXPECT_EQ(out.find(".-"), std::string::npos);
    EXPECT_NE(out.find("500.000000ms ago"), std::string::npos);
}

TEST_F(TimestampTest, HandlesFutureTime) {
    auto now = steady_clock::now().time_since_epoch();
    nanoseconds future_event = duration_cast<nanoseconds>(now + nanoseconds(1000000123LL));

    std::string out = ml::input_timestamp(future_event);

    EXPECT_EQ(out.find(".-"), std::string::npos);
    EXPECT_NE(out.find("in future"), std::string::npos);
    EXPECT_NE(out.find("1000.000123ms"), std::string::npos);
}