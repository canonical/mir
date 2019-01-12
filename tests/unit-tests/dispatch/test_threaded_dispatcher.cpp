/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/fd.h"
#include "mir/test/current_thread_name.h"
#include "mir/test/death.h"
#include "mir/test/pipe.h"
#include "mir/test/signal.h"
#include "mir/test/test_dispatchable.h"
#include "mir_test_framework/process.h"
#include "mir/test/cross_process_action.h"

#include <fcntl.h>

#include <atomic>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace md = mir::dispatch;
namespace mt = mir::test;

namespace
{
class ThreadedDispatcherTest : public ::testing::Test
{
public:
    ThreadedDispatcherTest()
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

TEST_F(ThreadedDispatcherTest, calls_dispatch_when_fd_is_readable)
{
    using namespace testing;

    auto dispatched = std::make_shared<mt::Signal>();
    auto dispatchable = std::make_shared<mt::TestDispatchable>([dispatched]() { dispatched->raise(); });

    md::ThreadedDispatcher dispatcher{"Ducks", dispatchable};

    dispatchable->trigger();

    EXPECT_TRUE(dispatched->wait_for(std::chrono::seconds{10}));
}

TEST_F(ThreadedDispatcherTest, stops_calling_dispatch_once_fd_is_not_readable)
{
    using namespace testing;

    std::atomic<int> dispatch_count{0};
    auto dispatchable = std::make_shared<mt::TestDispatchable>([&dispatch_count]() { ++dispatch_count; });

    md::ThreadedDispatcher dispatcher{"Dispatcher loop", dispatchable};

    dispatchable->trigger();

    // 1s is fine; if things are too slow we might get a false-pass, but that's OK.
    std::this_thread::sleep_for(std::chrono::seconds{1});

    EXPECT_THAT(dispatch_count, Eq(1));
}

TEST_F(ThreadedDispatcherTest, passes_dispatch_events_through)
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
    auto dispatchable = std::make_shared<mt::TestDispatchable>(delegate, md::FdEvent::readable | md::FdEvent::remote_closed);

    md::ThreadedDispatcher dispatcher{"Avocado", dispatchable};

    dispatchable->trigger();
    EXPECT_TRUE(dispatched_with_only_readable->wait_for(std::chrono::seconds{10}));

    dispatchable->hangup();
    EXPECT_TRUE(dispatched_with_hangup->wait_for(std::chrono::seconds{10}));
}

TEST_F(ThreadedDispatcherTest, doesnt_call_dispatch_after_first_false_return)
{
    using namespace testing;

    int constexpr expected_count{10};
    auto dispatched_more_than_enough = std::make_shared<mt::Signal>();
    std::atomic<int> dispatch_count{0};

    auto delegate = [dispatched_more_than_enough, &dispatch_count](md::FdEvents)
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
    auto dispatchable = std::make_shared<mt::TestDispatchable>(delegate);

    md::ThreadedDispatcher dispatcher{"Unexpected walrus", dispatchable};

    for (int i = 0; i < expected_count + 1; ++i)
    {
        dispatchable->trigger();
    }

    EXPECT_FALSE(dispatched_more_than_enough->wait_for(std::chrono::seconds{1}));
}

TEST_F(ThreadedDispatcherTest, only_calls_dispatch_with_remote_closed_when_relevant)
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

    md::ThreadedDispatcher dispatcher{"Implacable iguana", dispatchable};

    EXPECT_TRUE(dispatched_writable->wait_for(std::chrono::seconds{10}));

    // Make the fd remote-closed...
    watch_fd = mir::Fd{};
    EXPECT_FALSE(dispatched_closed->wait_for(std::chrono::seconds{1}));
}

TEST_F(ThreadedDispatcherTest, dispatches_multiple_dispatchees_simultaneously)
{
    using namespace testing;

    auto first_dispatched = std::make_shared<mt::Signal>();
    auto second_dispatched = std::make_shared<mt::Signal>();

    // Set up two dispatchables that can run given two threads of execution,
    // but will deadlock if run sequentially.
    auto first_dispatchable = std::make_shared<mt::TestDispatchable>([first_dispatched, second_dispatched]()
    {
        first_dispatched->raise();
        EXPECT_TRUE(second_dispatched->wait_for(std::chrono::seconds{60}));
    });
    auto second_dispatchable = std::make_shared<mt::TestDispatchable>([first_dispatched, second_dispatched]()
    {
        second_dispatched->raise();
        EXPECT_TRUE(first_dispatched->wait_for(std::chrono::seconds{60}));
    });

    auto combined_dispatchable = std::shared_ptr<md::MultiplexingDispatchable>(new md::MultiplexingDispatchable{first_dispatchable, second_dispatchable});
    md::ThreadedDispatcher dispatcher{"Barry", combined_dispatchable};

    dispatcher.add_thread();

    first_dispatchable->trigger();
    second_dispatchable->trigger();

    // Wait for dispatches to finish, so the dispatcher destructor doesn't race dispatching.
    EXPECT_TRUE(first_dispatched->wait_for(std::chrono::seconds{60}));
    EXPECT_TRUE(second_dispatched->wait_for(std::chrono::seconds{60}));
}

TEST_F(ThreadedDispatcherTest, remove_thread_decreases_concurrency)
{
    using namespace testing;

    // Set up two dispatchables that will fail if run simultaneously
    auto second_dispatched = std::make_shared<mt::Signal>();

    auto first_dispatchable = std::make_shared<mt::TestDispatchable>([second_dispatched]()
    {
        //1s is OK here; if things are slow, we might false-pass.
        EXPECT_FALSE(second_dispatched->wait_for(std::chrono::seconds{1}));
    });
    auto second_dispatchable = std::make_shared<mt::TestDispatchable>([second_dispatched]()
    {
        second_dispatched->raise();
    });

    auto combined_dispatchable = std::make_shared<md::MultiplexingDispatchable>();
    combined_dispatchable->add_watch(first_dispatchable);
    combined_dispatchable->add_watch(second_dispatchable);
    md::ThreadedDispatcher dispatcher{"Questionable decision", combined_dispatchable};

    dispatcher.add_thread();

    first_dispatchable->trigger();
    dispatcher.remove_thread();
    second_dispatchable->trigger();

    EXPECT_TRUE(second_dispatched->wait_for(std::chrono::seconds{10}));
}

using ThreadedDispatcherDeathTest = ThreadedDispatcherTest;

TEST_F(ThreadedDispatcherDeathTest, exceptions_in_threadpool_trigger_termination)
{
    using namespace testing;
    using namespace std::chrono_literals;

    constexpr char const* exception_msg = "Ducks! Ducks attack!";

    auto dispatchable = std::make_shared<mt::TestDispatchable>([]()
    {
        throw std::runtime_error{exception_msg};
    });

    dispatchable->trigger();

    // Ideally we'd use terminate_with_current_exception, but that's in server
    // and we're going to be called from a C context anyway, so just using the default
    // std::terminate behaviour should be acceptable.
    MIR_EXPECT_EXIT(
    {
        md::ThreadedDispatcher dispatcher("Die!", dispatchable);
        std::this_thread::sleep_for(10s);
    }, KilledBySignal(SIGABRT), (std::string{".*"} + exception_msg + ".*").c_str());
}

#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
TEST_F(ThreadedDispatcherTest, sets_thread_names_appropriately)
#else
TEST_F(ThreadedDispatcherTest, DISABLED_sets_thread_names_appropriately)
#endif
{
    using namespace testing;
    using namespace std::chrono_literals;

    auto dispatched = std::make_shared<mt::Signal>();
    constexpr int const threadcount{3};
    constexpr char const* threadname_base{"Madness Thread"};
    std::atomic<int> dispatch_count{0};

    auto dispatchable = std::make_shared<mt::TestDispatchable>([dispatched, &dispatch_count]()
    {
        EXPECT_THAT(mt::current_thread_name(), StartsWith(threadname_base));

        if (++dispatch_count == threadcount)
        {
            dispatched->raise();
        }
        else
        {
            dispatched->wait_for(10s);
        }
    });

    md::ThreadedDispatcher dispatcher{threadname_base, dispatchable};

    for (int i = 0; i < threadcount; ++i)
    {
        dispatcher.add_thread();
        dispatchable->trigger();
    }

    EXPECT_TRUE(dispatched->wait_for(10s));
}

void sigcont_handler(int)
{
}

// Regression test for: lp #1439719
TEST(ThreadedDispatcherSignalTest, keeps_dispatching_after_signal_interruption)
{
    using namespace std::chrono_literals;
    mt::CrossProcessAction stop_and_restart_process;
    mt::CrossProcessSync exit_success_sync;

    auto child = mir_test_framework::fork_and_run_in_a_different_process(
        [&]
        {
            {
                auto dispatched = std::make_shared<mt::Signal>();
                auto dispatchable = std::make_shared<mt::TestDispatchable>(
                    [dispatched]() { dispatched->raise(); });

                /* When there's no SIGCONT handler installed a SIGSTOP/SIGCONT
                 * pair POSIX mandates that a SIGSTOP/SIGCONT pair will *not*
                 * result in blocked syscalls returning EINTR.
                 *
                 * Linux isn't POSIX compliant for some of its syscalls -
                 * see “man 7 signal”, notably epoll - but for most syscalls
                 * it does the right thing.
                 *
                 * When there's a signal handler for SIGCONT installed then
                 * any blocked syscall will (correctly) return EINTR, so install one.
                 */
                signal(SIGCONT, &sigcont_handler);

                md::ThreadedDispatcher dispatcher{"Test thread", dispatchable};
                // Ensure the dispatcher has started
                dispatchable->trigger();
                EXPECT_TRUE(dispatched->wait_for(1s));

                stop_and_restart_process();

                dispatched->reset();
                // The dispatcher shouldn't have been affected by the signal
                dispatchable->trigger();
                EXPECT_TRUE(dispatched->wait_for(5s));

                if (!HasFailure())
                    exit_success_sync.signal_ready();

                // Because we terminate this process with an explicit call to
                // exit(), objects on the stack are not destroyed.
                // We need to destroy these objects manually to avoid fd leaks.
                stop_and_restart_process.~CrossProcessAction();
                exit_success_sync.~CrossProcessSync();
            }

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

    auto const result = child->wait_for_termination(30s);
    EXPECT_TRUE(result.exited_normally());

    // The test may run under valgrind which may change the exit code of the
    // forked child if it detects any issues. Issues detected by valgrind are
    // outside the scope of the test - they are for the parent caller to
    // examine and determine their validity; hence here we ignore the exit
    // code and instead use a sync object to determine if the child ran the
    // test successfully.
    // The child has already exited here so there's no reason to wait.
    auto const exit_success = exit_success_sync.wait_for_signal_ready_for(0s);
    EXPECT_TRUE(exit_success);
}

TEST_F(ThreadedDispatcherDeathTest, destroying_dispatcher_from_a_callback_is_an_error)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    MIR_EXPECT_EXIT(
    {
        std::atomic<md::ThreadedDispatcher*> dispatcher;

        auto dispatchable = std::make_shared<mt::TestDispatchable>(
            [&dispatcher]()
            {
                delete dispatcher.load();
            });

        dispatcher = new md::ThreadedDispatcher("Death thread", dispatchable);

        dispatchable->trigger();

        std::this_thread::sleep_for(10s);
    }, KilledBySignal(SIGABRT), ".*Destroying ThreadedDispatcher.*");
}

TEST_F(ThreadedDispatcherTest, executes_exception_handler_with_current_exception)
{
    using namespace testing;
    using namespace std::chrono_literals;
    auto dispatched = std::make_shared<mt::Signal>();
    std::exception_ptr exception;

    auto dispatchable = std::make_shared<mt::TestDispatchable>(
        []()
        {
            throw std::runtime_error("thrown");
        });

    md::ThreadedDispatcher dispatcher{"Walruses",
                                      dispatchable,
        [&dispatched,&exception]()
        {
            exception = std::current_exception();
            if (exception)
                dispatched->raise();
        }};
    dispatchable->trigger();
    EXPECT_TRUE(dispatched->wait_for(10s));
    EXPECT_THAT(exception, Not(Eq(nullptr)));
}
