/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "src/server/input/default_input_manager.h"

#include "mir/test/fd_utils.h"
#include "mir/test/signal.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_input_platform.h"

#include "mir/input/platform.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"

#include <sys/eventfd.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <list>

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
    mir::input::DefaultInputManager input_manager{mt::fake_shared(multiplexer)};

    DefaultInputManagerTest()
    {
        ON_CALL(platform, dispatchable())
            .WillByDefault(Return(mt::fake_shared(platform_dispatchable)));
    }

    bool wait_for_multiplexer_dispatch()
    {
        auto queue = std::make_shared<md::ActionQueue>();
        auto queue_done = std::make_shared<mt::Signal>();
        queue->enqueue([queue_done]()
                       {
                           queue_done->raise();
                       });
        multiplexer.add_watch(queue);
        bool ret = queue_done->wait_for(std::chrono::seconds{2});
        multiplexer.remove_watch(queue);
        return ret;
    }
};

}
TEST_F(DefaultInputManagerTest, starts_platforms_on_start)
{
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, dispatchable()).Times(1);

    input_manager.add_platform(mt::fake_shared(platform));
    input_manager.start();

    // start() is synchronous, all start-up operations should be finished at this point
    Mock::VerifyAndClearExpectations(&platform);

    EXPECT_CALL(platform, stop()).Times(1);
}

TEST_F(DefaultInputManagerTest, starts_platforms_after_start)
{
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, dispatchable()).Times(2);
    EXPECT_CALL(platform, stop()).Times(1);

    input_manager.start();
    input_manager.add_platform(mt::fake_shared(platform));

    EXPECT_TRUE(wait_for_multiplexer_dispatch());
}

TEST_F(DefaultInputManagerTest, stops_platforms_on_stop)
{
    EXPECT_CALL(platform, stop()).Times(1);

    input_manager.start();
    input_manager.add_platform(mt::fake_shared(platform));
    input_manager.stop();
}

TEST_F(DefaultInputManagerTest, ignores_spurious_starts)
{
    EXPECT_CALL(platform, start()).Times(1);

    input_manager.start();
    input_manager.add_platform(mt::fake_shared(platform));
    input_manager.start();

    EXPECT_TRUE(wait_for_multiplexer_dispatch());
}

TEST_F(DefaultInputManagerTest, ignores_spurious_stops)
{
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, stop()).Times(1);

    input_manager.start();
    input_manager.add_platform(mt::fake_shared(platform));
    input_manager.stop();
    input_manager.stop();
}
TEST_F(DefaultInputManagerTest, deals_with_parallel_starts)
{
    EXPECT_CALL(platform, start()).Times(1);
    const int more_than_one_thread = 10;

    input_manager.add_platform(mt::fake_shared(platform));
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

    input_manager.add_platform(mt::fake_shared(platform));
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
