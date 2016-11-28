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

    void fake_sleep_for(std::chrono::nanoseconds ns)
    {
        fake_time.nanoseconds += ns;
    }

    void fake_sleep_until(PosixTimestamp t)
    {
        ASSERT_EQ(t.clock_id, fake_time.clock_id);
        if (fake_time < t)
            fake_time = t;
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

    fake_sleep_until(b);
    fake_sleep_for(one_frame/13);  // short render time

    auto c = clock.next_frame_after(b);
    EXPECT_EQ(one_frame, c - b);

    fake_sleep_until(c);
    fake_sleep_for(one_frame/7);  // short render time

    auto d = clock.next_frame_after(c);
    EXPECT_EQ(one_frame, d - c);

    fake_sleep_until(d);
    fake_sleep_for(one_frame/5);  // short render time

    auto e = clock.next_frame_after(d);
    EXPECT_EQ(one_frame, e - d);
}

TEST_F(FrameClockTest, long_render_time_is_recoverable_without_decimation)
{
    FrameClock clock(with_fake_time);
    clock.set_period(one_frame);

    PosixTimestamp a = fake_time;
    auto b = clock.next_frame_after(a);

    fake_sleep_until(b);
    fake_sleep_for(one_frame * 5 / 4);  // long render time; over a frame

    auto c = clock.next_frame_after(b);
    EXPECT_EQ(one_frame, c - b);

    fake_sleep_until(c);
    fake_sleep_for(one_frame * 7 / 6);  // long render time; over a frame

    auto d = clock.next_frame_after(c);
    EXPECT_EQ(one_frame, d - c);

    EXPECT_LT(d, fake_time);

    fake_sleep_until(d);
    fake_sleep_for(one_frame/4);  // short render time

    // We can recover since we had a short render time...
    auto e = clock.next_frame_after(d);
    EXPECT_EQ(one_frame, e - d);
}

TEST_F(FrameClockTest, resuming_from_sleep_targets_the_future)
{
    FrameClock clock(with_fake_time);
    clock.set_period(one_frame);

    PosixTimestamp a = fake_time;
    auto b = clock.next_frame_after(a);
    fake_sleep_until(b);
    auto c = clock.next_frame_after(b);
    EXPECT_EQ(one_frame, c - b);
    fake_sleep_until(c);

    // Client idles for a while without producing new frames:
    fake_sleep_for(567 * one_frame);

    auto d = clock.next_frame_after(c);
    EXPECT_GT(d, fake_time);  // Resumption must be in the future
    EXPECT_LE(d, fake_time+one_frame);  // But not too far in the future
}

