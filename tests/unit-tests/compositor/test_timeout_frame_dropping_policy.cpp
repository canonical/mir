/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "src/server/compositor/timeout_frame_dropping_policy_factory.h"
#include "mir/compositor/frame_dropping_policy.h"
#include "mir/basic_callback.h"

#include "mir/test/doubles/mock_timer.h"
#include "mir/test/doubles/mock_lockable_callback.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/fake_shared.h"
#include "mir/test/gmock_fixes.h"

#include <stdexcept>
#include <mutex>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
class TimeoutFrameDroppingPolicy : public ::testing::Test
{
public:

    void SetUp() override
    {
       clock = std::make_shared<mt::FakeClock>();
       timer = std::make_shared<mtd::FakeTimer>(clock);
       handler = std::make_shared<mir::BasicCallback>([this]{ frame_dropped = true; });
    }

    auto create_default_policy()
    {
        mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
        return factory.create_policy(handler);
    }

protected:
    std::shared_ptr<mt::FakeClock> clock;
    std::shared_ptr<mtd::FakeTimer> timer;
    std::chrono::milliseconds const timeout = std::chrono::milliseconds(1000);
    std::shared_ptr<mir::BasicCallback> handler;
    bool frame_dropped = false;
};
}

TEST_F(TimeoutFrameDroppingPolicy, does_not_fire_before_notified_of_block)
{
    auto policy = create_default_policy();

    clock->advance_time(timeout + std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
}

TEST_F(TimeoutFrameDroppingPolicy, schedules_alarm_for_correct_timeout)
{
    auto policy = create_default_policy();
    policy->swap_now_blocking();

    clock->advance_time(timeout - std::chrono::milliseconds{1});
    EXPECT_FALSE(frame_dropped);
    clock->advance_time(std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);
}

TEST_F(TimeoutFrameDroppingPolicy, framedrop_callback_cancelled_by_unblock)
{
    auto policy = create_default_policy();

    policy->swap_now_blocking();
    policy->swap_unblocked();

    clock->advance_time(timeout * 10);

    EXPECT_FALSE(frame_dropped);
}

TEST_F(TimeoutFrameDroppingPolicy, policy_drops_one_frame_per_blocking_swap)
{
    auto policy = create_default_policy();

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

TEST_F(TimeoutFrameDroppingPolicy, policy_drops_frames_no_more_frequently_than_timeout)
{
    auto policy = create_default_policy();

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

TEST_F(TimeoutFrameDroppingPolicy, newly_blocking_frame_doesnt_reset_timeout)
{
    auto policy = create_default_policy();

    policy->swap_now_blocking();
    clock->advance_time(timeout - std::chrono::milliseconds{1});

    policy->swap_now_blocking();
    clock->advance_time(std::chrono::milliseconds{2});
    EXPECT_TRUE(frame_dropped);
}

TEST_F(TimeoutFrameDroppingPolicy, interspersed_timeouts_and_unblocks)
{
    auto policy = create_default_policy();

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

TEST_F(TimeoutFrameDroppingPolicy, policy_calls_lock_unlock_functions)
{
    using namespace testing;

    mc::TimeoutFrameDroppingPolicyFactory factory{timer, timeout};
    mtd::MockLockableCallback handler;
    {
        InSequence s;
        EXPECT_CALL(handler, lock());
        EXPECT_CALL(handler, functor());
        EXPECT_CALL(handler, unlock());
    }

    auto policy = factory.create_policy(mt::fake_shared(handler));

    policy->swap_now_blocking();
    clock->advance_time(timeout + std::chrono::milliseconds{1});
}
