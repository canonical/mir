/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "src/client/frame_clock.h"
#include <gtest/gtest.h>

using namespace ::testing;
using namespace ::std::chrono_literals;
using mir::time::PosixTimestamp;
using mir::client::FrameClock;

class FrameClockTest : public Test
{
public:
    FrameClockTest()
    {
        with_fake_time = [this](clockid_t){ return fake_time; };
    }

    void SetUp()
    {
        fake_time = PosixTimestamp();
        int const hz = 60;
        one_frame = std::chrono::nanoseconds(1000000000L/hz);
    }

protected:
    FrameClock::GetCurrentTime with_fake_time;
    PosixTimestamp fake_time;
    std::chrono::nanoseconds one_frame;
};

TEST_F(FrameClockTest, unthrottled_without_a_period)
{
    FrameClock clock(with_fake_time);
    PosixTimestamp a;
    auto b = clock.next_frame_after(a);
    EXPECT_EQ(a, b);
    auto c = clock.next_frame_after(b);
    EXPECT_EQ(b, c);
}

TEST_F(FrameClockTest, first_frame_is_within_one_frame)
{
    fake_time.nanoseconds = 12345678ns;
    auto a = fake_time;
    FrameClock clock(with_fake_time);
    clock.set_period(one_frame);
    PosixTimestamp b;
    auto c = clock.next_frame_after(b);
    EXPECT_GE(c, a);
    EXPECT_LE(c-a, one_frame);
}
