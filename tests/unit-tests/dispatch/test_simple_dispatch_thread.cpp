/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/dispatch/simple_dispatch_thread.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/fd.h"
#include "mir_test/pipe.h"
#include "mir_test/signal.h"

#include <fcntl.h>

#include <atomic>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace md = mir::dispatch;
namespace mt = mir::test;

namespace
{
class SimpleDispatchThreadTest : public ::testing::Test
{
public:
    SimpleDispatchThreadTest()
    {
        mt::Pipe pipe;
        watch_fd = pipe.read_fd();
        test_fd = pipe.write_fd();
        fcntl(watch_fd, F_SETFL, O_NONBLOCK);
    }

    mir::Fd watch_fd;
    mir::Fd test_fd;
};

class MockDispatchable : public md::Dispatchable
{
public:
    MOCK_CONST_METHOD0(watch_fd, mir::Fd());
    MOCK_METHOD1(dispatch, bool(md::FdEvents));
    MOCK_CONST_METHOD0(relevant_events, md::FdEvents());
};

}

TEST_F(SimpleDispatchThreadTest, calls_dispatch_when_fd_is_readable)
{
    using namespace testing;

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));
    ON_CALL(*dispatchable, relevant_events()).WillByDefault(Return(md::FdEvent::readable));

    auto dispatched = std::make_shared<mt::Signal>();
    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([dispatched](md::FdEvents) { dispatched->raise(); return true; }));

    md::SimpleDispatchThread dispatcher{dispatchable};

    uint64_t dummy{0xdeadbeef};
    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(dispatched->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleDispatchThreadTest, stops_calling_dispatch_once_fd_is_not_readable)
{
    using namespace testing;

    uint64_t dummy{0xdeadbeef};
    std::atomic<int> dispatch_count{0};

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));
    ON_CALL(*dispatchable, relevant_events()).WillByDefault(Return(md::FdEvent::readable));

    auto dispatched = std::make_shared<mt::Signal>();
    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([this, &dispatch_count](md::FdEvents)
    {
        decltype(dummy) buffer;
	dispatch_count++;
	EXPECT_THAT(read(this->watch_fd, &buffer, sizeof(buffer)), Eq(sizeof(buffer)));
	return true;
    }));

    md::SimpleDispatchThread dispatcher{dispatchable};

    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    std::this_thread::sleep_for(std::chrono::seconds{1});

    EXPECT_EQ(1, dispatch_count);
}

TEST_F(SimpleDispatchThreadTest, passes_dispatch_events_through)
{
    using namespace testing;

    uint64_t dummy{0xdeadbeef};
    std::atomic<int> dispatch_count{0};

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));
    ON_CALL(*dispatchable, relevant_events()).WillByDefault(Return(md::FdEvent::readable | md::FdEvent::remote_closed));

    auto dispatched_with_only_readable = std::make_shared<mt::Signal>();
    auto dispatched_with_hangup = std::make_shared<mt::Signal>();

    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([=](md::FdEvents events)
                                                     {
                                                         if (events == md::FdEvent::readable)
                                                         {
                                                             dispatched_with_only_readable->raise();
                                                         }
                                                         if (events & md::FdEvent::remote_closed)
                                                         {
                                                             dispatched_with_hangup->raise();
                                                             return false;
                                                         }
                                                         return true;
                                                     }));

    md::SimpleDispatchThread dispatcher{dispatchable};

    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(dispatched_with_only_readable->wait_for(std::chrono::seconds{1}));

    // This will trigger hangup
    test_fd = mir::Fd{};

    EXPECT_TRUE(dispatched_with_hangup->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleDispatchThreadTest, doesnt_call_dispatch_after_first_false_return)
{
    using namespace testing;

    uint64_t dummy{0xdeadbeef};

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));
    ON_CALL(*dispatchable, relevant_events()).WillByDefault(Return(md::FdEvent::readable));

    auto dispatched_more_than_enough = std::make_shared<mt::Signal>();

    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([dispatched_more_than_enough](md::FdEvents)
                                                     {
                                                          static std::atomic<int> dispatch_count{0};
                                                          int constexpr expected_count{10};

                                                          if (++dispatch_count == expected_count)
                                                          {
                                                              return false;
                                                          }
                                                          if (dispatch_count > expected_count)
                                                          {
                                                              dispatched_more_than_enough->raise();
                                                          }
                                                          return true;
                                                     }));

    md::SimpleDispatchThread dispatcher{dispatchable};

    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    EXPECT_FALSE(dispatched_more_than_enough->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleDispatchThreadTest, only_calls_dispatch_with_remote_closed_when_relevant)
{
    using namespace testing;

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(test_fd));
    ON_CALL(*dispatchable, relevant_events()).WillByDefault(Return(md::FdEvent::writable));
    auto dispatched_writable = std::make_shared<mt::Signal>();
    auto dispatched_closed = std::make_shared<mt::Signal>();

    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([=](md::FdEvents events)
    {
        if (events & md::FdEvent::writable)
        {
            dispatched_writable->raise();
        }
        if (events & md::FdEvent::remote_closed)
        {
            dispatched_closed->raise();
        }
        return true;
    }));

    md::SimpleDispatchThread dispatcher{dispatchable};

    EXPECT_TRUE(dispatched_writable->wait_for(std::chrono::seconds{1}));

    // Make the fd remote-closed...
    watch_fd = mir::Fd{};
    EXPECT_FALSE(dispatched_closed->wait_for(std::chrono::seconds{1}));
}
