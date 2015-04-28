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

#include "mir_test/fd_utils.h"
#include "mir_test/signal.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_input_platform.h"
#include "mir_test_doubles/mock_event_hub.h"
#include "mir_test_doubles/mock_input_reader.h"

#include "mir/input/platform.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"

#include <sys/eventfd.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
    NiceMock<mtd::MockEventHub> event_hub;
    NiceMock<mtd::MockInputReader> reader;
    mir::input::DefaultInputManager input_manager{mt::fake_shared(multiplexer), mt::fake_shared(reader), mt::fake_shared(event_hub)};

    DefaultInputManagerTest()
    {
        ON_CALL(event_hub, fd())
            .WillByDefault(Return(event_hub_fd));
        ON_CALL(platform, dispatchable())
            .WillByDefault(Return(mt::fake_shared(platform_dispatchable)));
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

TEST_F(DefaultInputManagerTest, flushes_event_hub_before_anything_to_fulfill_legacy_tests)
{
    testing::InSequence seq;
    EXPECT_CALL(event_hub, flush()).Times(1);
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, dispatchable()).Times(2);
    EXPECT_CALL(platform, stop()).Times(1);

    input_manager.add_platform(mt::fake_shared(platform));
    input_manager.start();
    Mock::VerifyAndClearExpectations(&event_hub);

    EXPECT_TRUE(wait_for_multiplexer_dispatch());
}

TEST_F(DefaultInputManagerTest, flushes_then_loops_once_to_initiate_device_scan_after_start)
{
    testing::InSequence seq;
    EXPECT_CALL(event_hub, flush()).Times(1);
    EXPECT_CALL(reader, loopOnce()).Times(1);

    input_manager.start();
    Mock::VerifyAndClearExpectations(&event_hub);

    EXPECT_TRUE(wait_for_multiplexer_dispatch());
}

TEST_F(DefaultInputManagerTest, starts_platforms_on_start)
{
    EXPECT_CALL(platform, start()).Times(1);
    EXPECT_CALL(platform, dispatchable()).Times(2);
    EXPECT_CALL(platform, stop()).Times(1);

    input_manager.add_platform(mt::fake_shared(platform));
    input_manager.start();

    EXPECT_TRUE(wait_for_multiplexer_dispatch());
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
