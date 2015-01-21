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
        watch_fd = mir::Fd{pipe.read_fd()};
        test_fd = mir::Fd{pipe.write_fd()};
        fcntl(watch_fd, F_SETFL, O_NONBLOCK);
    }

    mir::Fd watch_fd;
    mir::Fd test_fd;
private:
    mt::Pipe pipe;
};

class MockDispatchable : public md::Dispatchable
{
public:
    MOCK_CONST_METHOD0(watch_fd, mir::Fd());
    MOCK_METHOD1(dispatch, bool(md::fd_events));
    MOCK_CONST_METHOD0(relevant_events, md::fd_events());
};

}

TEST_F(SimpleDispatchThreadTest, CallsDispatchWhenFdIsReadable)
{
    using namespace testing;

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));

    auto dispatched = std::make_shared<mt::Signal>();
    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([dispatched](md::fd_events) { dispatched->raise(); return true; }));

    md::SimpleDispatchThread dispatcher{dispatchable};

    uint64_t dummy{0xdeadbeef};
    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(dispatched->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleDispatchThreadTest, StopsCallingDispatchOnceFdIsNotReadable)
{
    using namespace testing;

    uint64_t dummy{0xdeadbeef};
    std::atomic<int> dispatch_count{0};

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));

    auto dispatched = std::make_shared<mt::Signal>();
    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([this, &dispatch_count](md::fd_events)
                                                     {
                                                         decltype(dummy) buffer;
                                                         dispatch_count++;
                                                         read(this->watch_fd, &buffer, sizeof(buffer));
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

    auto dispatched_with_only_readable = std::make_shared<mt::Signal>();
    auto dispatched_with_hangup = std::make_shared<mt::Signal>();

    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([=](md::fd_events events)
                                                     {
                                                         if (events == md::fd_event::readable)
                                                         {
                                                             dispatched_with_only_readable->raise();
                                                         }
                                                         if (events & md::fd_event::remote_closed)
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
    ::close(test_fd);

    EXPECT_TRUE(dispatched_with_hangup->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleDispatchThreadTest, doesnt_call_dispatch_after_first_false_return)
{
    using namespace testing;

    uint64_t dummy{0xdeadbeef};

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));

    auto dispatched_more_than_enough = std::make_shared<mt::Signal>();

    ON_CALL(*dispatchable, dispatch(_)).WillByDefault(Invoke([dispatched_more_than_enough](md::fd_events)
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
