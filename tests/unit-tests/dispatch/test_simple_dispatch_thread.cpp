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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/dispatch/simple_dispatch_thread.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/fd.h"
#include "mir_test/pipe.h"
#include "mir_test/signal.h"
#include "mir_test/test_dispatchable.h"
#include "mir_test_framework/process.h"
#include "mir_test/cross_process_action.h"

#include <fcntl.h>

#include <atomic>
#include <exception>
#include <thread>

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
        mt::Pipe pipe{O_NONBLOCK};
        watch_fd = pipe.read_fd();
        test_fd = pipe.write_fd();
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

    auto dispatched = std::make_shared<mt::Signal>();
    auto dispatchable = std::make_shared<mt::TestDispatchable>([dispatched]()
                                                               {
                                                                   dispatched->raise();
                                                               });

    md::SimpleDispatchThread dispatcher{dispatchable};

    dispatchable->trigger();

    EXPECT_TRUE(dispatched->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleDispatchThreadTest, stops_calling_dispatch_once_fd_is_not_readable)
{
    using namespace testing;

    std::atomic<int> dispatch_count{0};
    auto dispatchable = std::make_shared<mt::TestDispatchable>([&dispatch_count]()
                                                               {
                                                                   ++dispatch_count;
                                                               });

    md::SimpleDispatchThread dispatcher{dispatchable};

    dispatchable->trigger();

    std::this_thread::sleep_for(std::chrono::seconds{1});

    EXPECT_THAT(dispatch_count, Eq(1));
}

TEST_F(SimpleDispatchThreadTest, passes_dispatch_events_through)
{
    using namespace testing;

    auto dispatched_with_only_readable = std::make_shared<mt::Signal>();
    auto dispatched_with_hangup = std::make_shared<mt::Signal>();
    auto delegate = [dispatched_with_only_readable, dispatched_with_hangup](md::FdEvents events)
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
    };
    auto dispatchable =
        std::make_shared<mt::TestDispatchable>(delegate, md::FdEvent::readable | md::FdEvent::remote_closed);

    md::SimpleDispatchThread dispatcher{dispatchable};

    dispatchable->trigger();
    EXPECT_TRUE(dispatched_with_only_readable->wait_for(std::chrono::seconds{1}));

    dispatchable->hangup();
    EXPECT_TRUE(dispatched_with_hangup->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleDispatchThreadTest, doesnt_call_dispatch_after_first_false_return)
{
    using namespace testing;
    using namespace std::chrono_literals;

    int constexpr expected_count{10};
    auto const dispatched_more_than_enough = std::make_shared<mt::Signal>();

    auto delegate =
        [dispatched_more_than_enough, dispatch_count = 0](md::FdEvents) mutable
        {
            if (++dispatch_count == expected_count)
            {
                return false;
            }
            if (dispatch_count > expected_count)
            {
                dispatched_more_than_enough->raise();
            }
            return true;
        };
    auto const dispatchable = std::make_shared<mt::TestDispatchable>(delegate);

    md::SimpleDispatchThread dispatcher{dispatchable};

    for (int i = 0; i < expected_count + 1; ++i)
    {
        dispatchable->trigger();
    }

    EXPECT_FALSE(dispatched_more_than_enough->wait_for(1s));
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

// Regression test for: lp #1439719
// The bug involves uninitialized memory and is also sensitive to signal
// timings, so this test does not always catch the problem. However, repeated
// runs (~300, YMMV) consistently fail when run against the problematic code.
TEST_F(SimpleDispatchThreadTest, keeps_dispatching_after_signal_interruption)
{
    using namespace std::chrono_literals;
    mt::CrossProcessAction stop_and_restart_process;

    auto child = mir_test_framework::fork_and_run_in_a_different_process(
        [&]
        {
            auto dispatched = std::make_shared<mt::Signal>();
            auto dispatchable = std::make_shared<mt::TestDispatchable>(
                [dispatched]() { dispatched->raise(); });

            md::SimpleDispatchThread dispatcher{dispatchable};
            // Ensure the dispatcher has started
            dispatchable->trigger();
            EXPECT_TRUE(dispatched->wait_for(1s));

            stop_and_restart_process();

            dispatched->reset();
            // The dispatcher shouldn't have been affected by the signal
            dispatchable->trigger();
            EXPECT_TRUE(dispatched->wait_for(1s));
            exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
        },
        []{ return 1; });

    stop_and_restart_process.exec(
        [child]
        {
            // Increase chances of interrupting the dispatch mechanism
            for (int i = 0; i < 100; ++i)
            {
                child->stop();
                child->cont();
            }
        });

    auto const result = child->wait_for_termination(10s);
    EXPECT_TRUE(result.succeeded());
}

using SimpleDispatchThreadDeathTest = SimpleDispatchThreadTest;

TEST_F(SimpleDispatchThreadDeathTest, destroying_dispatcher_from_a_callback_is_an_error)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    EXPECT_EXIT(
    {
        std::mutex mutex;
        md::SimpleDispatchThread* dispatcher;
    
        auto dispatchable = std::make_shared<mt::TestDispatchable>([&dispatcher, &mutex]{
            std::lock_guard<decltype(mutex)> lock{mutex};
            delete dispatcher;
        });
        
        {
            std::lock_guard<decltype(mutex)> lock{mutex};
            dispatchable->trigger();
            dispatcher = new md::SimpleDispatchThread{dispatchable};
        }
        std::this_thread::sleep_for(10s);
    }, KilledBySignal(SIGABRT), ".*Destroying SimpleDispatchThread.*");
}

TEST_F(SimpleDispatchThreadTest, executes_exception_handler_with_current_exception)
{
    using namespace std::chrono_literals;
    auto dispatched = std::make_shared<mt::Signal>();
    std::exception_ptr exception;

    auto dispatchable = std::make_shared<mt::TestDispatchable>(
        []()
        {
            throw std::runtime_error("thrown");
        });

    md::SimpleDispatchThread dispatcher{dispatchable,
        [&dispatched,&exception]()
        {
            exception = std::current_exception();
            if (exception)
                dispatched->raise();
        }};
    dispatchable->trigger();
    EXPECT_TRUE(dispatched->wait_for(10s));
    EXPECT_TRUE(exception!=nullptr);
}
