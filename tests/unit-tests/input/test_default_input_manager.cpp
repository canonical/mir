/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/input/default_input_manager.h"

#include <mir/test/signal_actions.h>
#include <mir/test/fake_shared.h>
#include <mir/test/doubles/mock_input_platform.h>

#include <mir/dispatch/multiplexing_dispatchable.h>
#include <mir/dispatch/action_queue.h>

#include <sys/eventfd.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <atomic>
#include <chrono>
#include <list>
#include <thread>

namespace mt = mir::test;
namespace md = mir::dispatch;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{
struct DefaultInputManagerTest : ::testing::Test
{
    md::MultiplexingDispatchable multiplexer;
    md::ActionQueue platform_dispatchable;
    NiceMock<mtd::MockInputPlatform> platform;
    mir::Fd event_hub_fd{eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK)};
    mir::input::DefaultInputManager input_manager{mt::fake_shared(multiplexer), mt::fake_shared(platform)};
    std::chrono::seconds const timeout{30};

    DefaultInputManagerTest()
    {
        ON_CALL(platform, dispatchable())
            .WillByDefault(Return(mt::fake_shared(platform_dispatchable)));
    }
};

}
TEST_F(DefaultInputManagerTest, starts_platforms_on_start)
{
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, dispatchable()).Times(1);

    input_manager.start();

    // start() is synchronous, all start-up operations should be finished at this point
    Mock::VerifyAndClearExpectations(&platform);

    EXPECT_CALL(platform, stop()).Times(1);
}

TEST_F(DefaultInputManagerTest, stops_platforms_on_stop)
{
    EXPECT_CALL(platform, stop()).Times(1);

    input_manager.start();
    input_manager.stop();
}

TEST_F(DefaultInputManagerTest, ignores_spurious_starts)
{
    EXPECT_CALL(platform, start()).Times(1);

    input_manager.start();
    input_manager.start();
}

TEST_F(DefaultInputManagerTest, ignores_spurious_stops)
{
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, stop()).Times(1);

    input_manager.start();
    input_manager.stop();
    input_manager.stop();
}
TEST_F(DefaultInputManagerTest, deals_with_parallel_starts)
{
    EXPECT_CALL(platform, start()).Times(1);
    const int more_than_one_thread = 10;

    std::list<std::thread> threads;
    for (int i = 0; i != more_than_one_thread; ++i)
    {
        threads.emplace_back([this]()
                             {
                                 input_manager.start();
                             });
    }
    while (!threads.empty())
    {
        threads.front().join();
        threads.pop_front();
    }
}

TEST_F(DefaultInputManagerTest, deals_with_parallel_stops)
{
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, stop()).Times(1);
    const int more_than_one_thread = 10;

    input_manager.start();
    std::list<std::thread> threads;
    for (int i = 0; i != more_than_one_thread; ++i)
    {
        threads.emplace_back([this]()
                             {
                                 input_manager.stop();
                             });
    }
    while (!threads.empty())
    {
        threads.front().join();
        threads.pop_front();
    }
}

TEST_F(DefaultInputManagerTest, forwards_pause_continue_state_changes_to_platform)
{
    input_manager.start();

    mt::Signal paused;
    EXPECT_CALL(platform, pause_for_config()).WillOnce(WakeUp(&paused));
    input_manager.pause_for_config();
    EXPECT_TRUE(paused.wait_for(timeout));

    mt::Signal continued;
    EXPECT_CALL(platform, continue_after_config()).WillOnce(WakeUp(&continued));
    input_manager.continue_after_config();
    EXPECT_TRUE(continued.wait_for(timeout));
}

TEST_F(DefaultInputManagerTest, calls_platform_methods_on_a_single_dedicated_thread)
{
    std::atomic<std::thread::id> platform_thread{};
    auto check_thread = [&]
    {
        auto const current_thread = std::this_thread::get_id();
        auto expected_thread = std::thread::id{};

        // First time through here we replace the empty default value for `platform_thread` with the current thread.
        platform_thread.compare_exchange_strong(expected_thread, current_thread);

        EXPECT_THAT(current_thread, Eq(platform_thread.load()));
    };

    EXPECT_CALL(platform, start()).Times(AtLeast(1)).WillRepeatedly(check_thread);
    EXPECT_CALL(platform, stop()).Times(AtLeast(1)).WillRepeatedly(check_thread);
    EXPECT_CALL(platform, pause_for_config()).Times(AtLeast(1)).WillRepeatedly(check_thread);
    EXPECT_CALL(platform, continue_after_config()).Times(AtLeast(1)).WillRepeatedly(check_thread);
    EXPECT_CALL(platform, dispatchable()).Times(AtLeast(1)).WillRepeatedly([&]
        {
            check_thread();
            return mt::fake_shared(platform_dispatchable);
        });

    auto const main_thread = std::this_thread::get_id();
    input_manager.start();
    mt::Signal dispatched;
    platform_dispatchable.enqueue([&]
        {
            check_thread();
            dispatched.raise();
        });
    EXPECT_TRUE(dispatched.wait_for(std::chrono::seconds{2}));

    std::jthread caller{[&]
        {
            input_manager.pause_for_config();
            input_manager.continue_after_config();
            input_manager.stop();
        }};
    auto const caller_thread = caller.get_id();
    caller.join();

    EXPECT_THAT(platform_thread.load(), Ne(main_thread));
    EXPECT_THAT(platform_thread.load(), Ne(caller_thread));
}
