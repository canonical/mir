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
        fake_time = PosixTimestamp(CLOCK_MONOTONIC, 12345678ns);
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
    auto a = fake_time;
    FrameClock clock(with_fake_time);
    clock.set_period(one_frame);

    PosixTimestamp b;
    auto c = clock.next_frame_after(b);
    EXPECT_GE(c, a);
    EXPECT_LE(c-a, one_frame);
}

TEST_F(FrameClockTest, interval_is_perfectly_smooth)
{
    FrameClock clock(with_fake_time);
    clock.set_period(one_frame);

    PosixTimestamp a;
    auto b = clock.next_frame_after(a);

    // Render time varies but our interval should not...
    fake_time.nanoseconds += one_frame/13;

    auto c = clock.next_frame_after(b);
    EXPECT_EQ(one_frame, c - b);

    fake_time.nanoseconds += one_frame/7;

    auto d = clock.next_frame_after(c);
    EXPECT_EQ(one_frame, d - c);

    fake_time.nanoseconds += one_frame/5;

    auto e = clock.next_frame_after(d);
    EXPECT_EQ(one_frame, e - d);
}

TEST_F(FrameClockTest, long_render_time_is_recoverable_without_decimation)
{
    FrameClock clock(with_fake_time);
    clock.set_period(one_frame);

    PosixTimestamp a = fake_time;
    auto b = clock.next_frame_after(a);

    fake_time.nanoseconds += one_frame * 5 / 4;

    auto c = clock.next_frame_after(b);
    EXPECT_EQ(one_frame, c - b);

    fake_time.nanoseconds += one_frame * 7 / 6;

    auto d = clock.next_frame_after(c);
    EXPECT_EQ(one_frame, d - c);

    EXPECT_LT(d, fake_time);
}

TEST_F(FrameClockTest, resuming_from_sleep_targets_the_future)
{
    FrameClock clock(with_fake_time);
    clock.set_period(one_frame);

    PosixTimestamp a = fake_time;
    auto b = clock.next_frame_after(a);
    auto c = clock.next_frame_after(b);
    EXPECT_EQ(one_frame, c - b);

    // Client idles for a while without producing new frames:
    fake_time.nanoseconds += 567 * one_frame;

    auto d = clock.next_frame_after(c);
    EXPECT_GT(d, fake_time);  // Resumption must be in the future
    EXPECT_LE(d, fake_time+one_frame);  // But not too far in the future
}

