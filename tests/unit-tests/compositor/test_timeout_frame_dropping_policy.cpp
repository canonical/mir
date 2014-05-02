/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/server/compositor/timeout_frame_dropping_policy.h"

#include "mir_test_doubles/mock_timer.h"

#include "mir_test/gmock_fixes.h"

#include <stdexcept>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(TimeoutFrameDroppingPolicy, schedules_alarm_for_correct_timeout)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::MockTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    mc::TimeoutFrameDroppingPolicy policy{timer, timeout};
    bool frame_dropped{false};
    policy.swap_now_blocking([&frame_dropped]{ frame_dropped = true; });

    clock->advance_time(timeout - std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
    clock->advance_time(std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, blocking_while_a_block_is_pending_is_an_error)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::MockTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    mc::TimeoutFrameDroppingPolicy policy{timer, timeout};
    policy.swap_now_blocking([]{});
    EXPECT_THROW({ policy.swap_now_blocking([]{}); }, std::logic_error);
}

TEST(TimeoutFrameDroppingPolicy, framedrop_callback_cancelled_by_unblock)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::MockTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    mc::TimeoutFrameDroppingPolicy policy{timer, timeout};
    bool frame_dropped{false};
    auto cookie = policy.swap_now_blocking([&frame_dropped](){ frame_dropped = true; });

    policy.swap_unblocked(cookie);

    clock->advance_time(timeout * 10);

    EXPECT_FALSE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, calls_correct_callback_after_timeout)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::MockTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    mc::TimeoutFrameDroppingPolicy policy{timer, timeout};
    bool frame_dropped[3] = {false,};
    policy.swap_now_blocking([&frame_dropped]{ frame_dropped[0] = true; });

    clock->advance_time(timeout + std::chrono::milliseconds{1});
    policy.swap_now_blocking([&frame_dropped]{ frame_dropped[1] = true; });
    EXPECT_TRUE(frame_dropped[0]);
    EXPECT_FALSE(frame_dropped[1]);
    EXPECT_FALSE(frame_dropped[2]);

    clock->advance_time(timeout + std::chrono::milliseconds{1});
    policy.swap_now_blocking([&frame_dropped]{ frame_dropped[2] = true; });
    EXPECT_TRUE(frame_dropped[0]);
    EXPECT_TRUE(frame_dropped[1]);
    EXPECT_FALSE(frame_dropped[2]);

    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_TRUE(frame_dropped[0]);
    EXPECT_TRUE(frame_dropped[1]);
    EXPECT_TRUE(frame_dropped[2]);
}
