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

#include "src/server/compositor/timeout_frame_dropping_policy_factory.h"
#include "mir/compositor/frame_dropping_policy.h"

#include "mir_test_doubles/mock_timer.h"

#include "mir_test/gmock_fixes.h"

#include <stdexcept>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(TimeoutFrameDroppingPolicy, does_not_fire_before_notified_of_block)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::FakeTimer>(clock);
    std::chrono::milliseconds const timeout{1000};
    bool frame_dropped{false};

    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    auto policy = factory.create_policy([&frame_dropped]{ frame_dropped = true; });

    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, schedules_alarm_for_correct_timeout)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::FakeTimer>(clock);
    std::chrono::milliseconds const timeout{1000};
    bool frame_dropped{false};

    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    auto policy = factory.create_policy([&frame_dropped]{ frame_dropped = true; });
    policy->swap_now_blocking();

    clock->advance_time(timeout - std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
    clock->advance_time(std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, framedrop_callback_cancelled_by_unblock)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::FakeTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    bool frame_dropped{false};
    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    auto policy = factory.create_policy([&frame_dropped]{ frame_dropped = true; });

    policy->swap_now_blocking();
    policy->swap_unblocked();

    clock->advance_time(timeout * 10);

    EXPECT_FALSE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, policy_drops_one_frame_per_blocking_swap)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::FakeTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    bool frame_dropped{false};
    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    auto policy = factory.create_policy([&frame_dropped]{ frame_dropped = true; });

    policy->swap_now_blocking();
    policy->swap_now_blocking();
    policy->swap_now_blocking();

    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_TRUE(frame_dropped);

    frame_dropped = false;
    clock->advance_time(timeout + std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);

    frame_dropped = false;
    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_TRUE(frame_dropped);

    frame_dropped = false;
    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, policy_drops_frames_no_more_frequently_than_timeout)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::FakeTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    bool frame_dropped{false};
    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    auto policy = factory.create_policy([&frame_dropped]{ frame_dropped = true; });

    policy->swap_now_blocking();
    policy->swap_now_blocking();

    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_TRUE(frame_dropped);

    frame_dropped = false;
    clock->advance_time(timeout - std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
    clock->advance_time(std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, newly_blocking_frame_doesnt_reset_timeout)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::FakeTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    bool frame_dropped{false};
    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    auto policy = factory.create_policy([&frame_dropped]{ frame_dropped = true; });

    policy->swap_now_blocking();
    clock->advance_time(timeout - std::chrono::milliseconds{1});

    policy->swap_now_blocking();
    clock->advance_time(std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);
}

TEST(TimeoutFrameDroppingPolicy, interspersed_timeouts_and_unblocks)
{
    auto clock = std::make_shared<mt::FakeClock>();
    auto timer = std::make_shared<mtd::FakeTimer>(clock);
    std::chrono::milliseconds const timeout{1000};

    bool frame_dropped{false};
    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    auto policy = factory.create_policy([&frame_dropped]{ frame_dropped = true; });

    policy->swap_now_blocking();
    policy->swap_now_blocking();
    policy->swap_now_blocking();

    /* First frame gets dropped... */
    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_TRUE(frame_dropped);

    /* ...Compositor gets its act in order and consumes a frame... */
    frame_dropped = false;
    policy->swap_unblocked();
    clock->advance_time(timeout - std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);

    /* ...but not a second frame, so third swap should trigger a timeout */
    clock->advance_time(std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);

    frame_dropped = false;
    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
}
