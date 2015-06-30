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

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/fd.h"
#include "mir/test/pipe.h"
#include "mir/test/signal.h"
#include "mir/test/fd_utils.h"
#include "mir/test/test_dispatchable.h"
#include "mir/test/auto_unblock_thread.h"

#include <fcntl.h>

#include <atomic>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace md = mir::dispatch;
namespace mt = mir::test;

TEST(MultiplexingDispatchableTest, dispatches_dispatchee_when_appropriate)
{
    bool dispatched{false};
    auto dispatchee = std::make_shared<mt::TestDispatchable>([&dispatched]() { dispatched = true; });
    md::MultiplexingDispatchable dispatcher{dispatchee};

    dispatchee->trigger();

    ASSERT_TRUE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_TRUE(dispatched);
}

TEST(MultiplexingDispatchableTest, calls_correct_dispatchee_when_fd_becomes_readable)
{
    bool a_dispatched{false};
    auto dispatchee_a = std::make_shared<mt::TestDispatchable>([&a_dispatched]() { a_dispatched = true; });

    bool b_dispatched{false};
    auto dispatchee_b = std::make_shared<mt::TestDispatchable>([&b_dispatched]() { b_dispatched = true; });

    md::MultiplexingDispatchable dispatcher{dispatchee_a, dispatchee_b};

    dispatchee_a->trigger();

    ASSERT_TRUE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_TRUE(a_dispatched);
    EXPECT_FALSE(b_dispatched);

    ASSERT_FALSE(mt::fd_is_readable(dispatcher.watch_fd()));
    a_dispatched = false;

    dispatchee_b->trigger();

    ASSERT_TRUE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_FALSE(a_dispatched);
    EXPECT_TRUE(b_dispatched);
}

TEST(MultiplexingDispatchableTest, keeps_dispatching_until_fd_is_unreadable)
{
    bool dispatched{false};
    auto dispatchee = std::make_shared<mt::TestDispatchable>([&dispatched]() { dispatched = true; });
    md::MultiplexingDispatchable dispatcher{dispatchee};

    int const trigger_count{10};

    for (int i = 0; i < trigger_count; ++i)
    {
        dispatchee->trigger();
    }

    for (int i = 0; i < trigger_count; ++i)
    {
        ASSERT_TRUE(mt::fd_is_readable(dispatcher.watch_fd()));
        dispatcher.dispatch(md::FdEvent::readable);

        EXPECT_TRUE(dispatched);
        dispatched = false;
    }

    EXPECT_FALSE(mt::fd_is_readable(dispatcher.watch_fd()));
}

TEST(MultiplexingDispatchableTest, dispatching_without_pending_event_is_harmless)
{
    bool dispatched{false};
    auto dispatchee = std::make_shared<mt::TestDispatchable>([&dispatched]() { dispatched = true; });
    md::MultiplexingDispatchable dispatcher{dispatchee};

    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_FALSE(dispatched);
}

TEST(MultiplexingDispatchableTest, keeps_dispatchees_alive)
{
    bool dispatched{false};
    auto dispatchee = std::make_shared<mt::TestDispatchable>([&dispatched]() { dispatched = true; });
    dispatchee->trigger();

    md::MultiplexingDispatchable dispatcher;
    dispatcher.add_watch(dispatchee);
    dispatchee.reset();

    ASSERT_TRUE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_TRUE(dispatched);
}

TEST(MultiplexingDispatchableTest, removed_dispatchables_are_no_longer_dispatched)
{
    using namespace testing;

    mt::Pipe pipe;

    bool dispatched{false};
    auto dispatchable = std::make_shared<mt::TestDispatchable>([&dispatched]() { dispatched = true; });

    md::MultiplexingDispatchable dispatcher;
    dispatcher.add_watch(dispatchable);
    dispatcher.remove_watch(dispatchable);

    while (mt::fd_is_readable(dispatcher.watch_fd()))
    {
        dispatcher.dispatch(md::FdEvent::readable);
    }

    dispatchable->trigger();

    EXPECT_FALSE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_FALSE(dispatched);
}

TEST(MultiplexingDispatchableTest, adding_same_fd_twice_is_an_error)
{
    using namespace testing;

    auto dispatchable = std::make_shared<mt::TestDispatchable>([](){});

    md::MultiplexingDispatchable dispatcher;
    dispatcher.add_watch(dispatchable);

    EXPECT_THROW(dispatcher.add_watch(dispatchable),
                 std::logic_error);
}

TEST(MultiplexingDispatchableTest, dispatcher_does_not_hold_reference_after_failing_to_add_dispatchee)
{
    using namespace testing;

    auto dispatchable = std::make_shared<mt::TestDispatchable>([](){});

    md::MultiplexingDispatchable dispatcher;
    dispatcher.add_watch(dispatchable);

    auto const dispatchable_refcount = dispatchable.use_count();

    // This should not increase refcount
    EXPECT_THROW(dispatcher.add_watch(dispatchable),
                 std::logic_error);
    EXPECT_THAT(dispatchable.use_count(), Eq(dispatchable_refcount));
}

TEST(MultiplexingDispatchableTest, individual_dispatchee_is_not_concurrent)
{
    using namespace testing;

    auto second_dispatch = std::make_shared<mt::Signal>();
    auto dispatchee = std::make_shared<mt::TestDispatchable>([second_dispatch]()
    {
        static std::atomic<int> canary{0};
        static std::atomic<int> total_count{0};

        ++canary;
        EXPECT_THAT(canary, Eq(1));
        if (++total_count == 2)
        {
            second_dispatch->raise();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
        --canary;
    });

    dispatchee->trigger();
    dispatchee->trigger();

    auto dispatcher = std::make_shared<md::MultiplexingDispatchable>();
    dispatcher->add_watch(dispatchee);

    md::ThreadedDispatcher eventloop{"Gerry", dispatcher};
    eventloop.add_thread();

    EXPECT_TRUE(second_dispatch->wait_for(std::chrono::seconds{5}));
}

TEST(MultiplexingDispatchableTest, reentrant_dispatchee_is_dispatched_concurrently)
{
    using namespace testing;

    std::atomic<int> count{0};

    auto dispatchee = std::make_shared<mt::TestDispatchable>([&count]()
    {
        ++count;
        std::this_thread::sleep_for(std::chrono::seconds{1});
        EXPECT_THAT(count, Gt(1));
    });

    dispatchee->trigger();
    dispatchee->trigger();

    md::MultiplexingDispatchable dispatcher;
    dispatcher.add_watch(dispatchee, md::DispatchReentrancy::reentrant);

    std::thread first{[&dispatcher]() { dispatcher.dispatch(md::FdEvent::readable); }};
    std::thread second{[&dispatcher]() { dispatcher.dispatch(md::FdEvent::readable); }};

    first.join();
    second.join();
}

TEST(MultiplexingDispatchableTest, raw_callback_is_dispatched)
{
    using namespace testing;

    bool dispatched{false};
    auto dispatchee = [&dispatched]() { dispatched = true; };
    mt::Pipe fd_source;

    md::MultiplexingDispatchable dispatcher;
    dispatcher.add_watch(fd_source.read_fd(), dispatchee);

    char buffer{0};
    ASSERT_THAT(::write(fd_source.write_fd(), &buffer, sizeof(buffer)), Eq(sizeof(buffer)));

    EXPECT_TRUE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_TRUE(dispatched);
}

TEST(MultiplexingDispatchableTest, raw_callback_can_be_removed)
{
    using namespace testing;

    bool dispatched{false};
    auto dispatchee = [&dispatched]() { dispatched = true; };
    mt::Pipe fd_source;

    md::MultiplexingDispatchable dispatcher;
    dispatcher.add_watch(fd_source.read_fd(), dispatchee);
    dispatcher.remove_watch(fd_source.read_fd());

    while (mt::fd_is_readable(dispatcher.watch_fd()))
    {
        dispatcher.dispatch(md::FdEvent::readable);
    }

    char buffer{0};
    ASSERT_THAT(::write(fd_source.write_fd(), &buffer, sizeof(buffer)), Eq(sizeof(buffer)));

    EXPECT_FALSE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_FALSE(dispatched);
}

TEST(MultiplexingDispatchableTest, removal_is_threadsafe)
{
    using namespace testing;

    auto canary_killed = std::make_shared<mt::Signal>();
    auto canary = std::shared_ptr<int>(new int, [canary_killed](int* victim) { delete victim; canary_killed->raise(); });
    auto in_dispatch = std::make_shared<mt::Signal>();

    auto dispatcher = std::make_shared<md::MultiplexingDispatchable>();

    auto dispatchee = std::make_shared<mt::TestDispatchable>([canary, in_dispatch]()
    {
        in_dispatch->raise();
        std::this_thread::sleep_for(std::chrono::seconds{1});
        EXPECT_THAT(canary.use_count(), Gt(0));
    });
    dispatcher->add_watch(dispatchee);

    dispatchee->trigger();

    md::ThreadedDispatcher eventloop{"Tempura", dispatcher};

    EXPECT_TRUE(in_dispatch->wait_for(std::chrono::seconds{1}));

    dispatcher->remove_watch(dispatchee);
    dispatchee.reset();
    canary.reset();

    EXPECT_TRUE(canary_killed->wait_for(std::chrono::seconds{2}));
}

TEST(MultiplexingDispatchableTest, destruction_is_threadsafe)
{
    using namespace testing;

    auto canary_killed = std::make_shared<mt::Signal>();
    auto canary = std::shared_ptr<int>(new int, [canary_killed](int* victim) { delete victim; canary_killed->raise(); });
    auto in_dispatch = std::make_shared<mt::Signal>();

    auto dispatcher = std::make_shared<md::MultiplexingDispatchable>();

    auto dispatchee = std::make_shared<mt::TestDispatchable>([canary, in_dispatch]()
    {
        in_dispatch->raise();
        std::this_thread::sleep_for(std::chrono::seconds{1});
        EXPECT_THAT(canary.use_count(), Gt(0));
    });
    dispatcher->add_watch(dispatchee);

    dispatchee->trigger();

    mt::AutoJoinThread dispatch_thread{[dispatcher]() { dispatcher->dispatch(md::FdEvent::readable); }};

    EXPECT_TRUE(in_dispatch->wait_for(std::chrono::seconds{1}));

    dispatcher->remove_watch(dispatchee);
    dispatcher.reset();
    dispatchee.reset();
    canary.reset();

    EXPECT_TRUE(canary_killed->wait_for(std::chrono::seconds{2}));
}

TEST(MultiplexingDispatchableTest, stress_test_threading)
{
    using namespace testing;

    int const dispatchee_count{20};

    auto dispatcher = std::make_shared<md::MultiplexingDispatchable>();

    auto event_dispatcher = std::make_shared<md::ThreadedDispatcher>("Hello Kitty", dispatcher);
    for (int i = 0 ; i < dispatchee_count + 5 ; ++i)
    {
        event_dispatcher->add_thread();
    }

    std::vector<std::shared_ptr<mt::Signal>> canary_tomb;
    std::vector<std::shared_ptr<mt::TestDispatchable>> dispatchees;
    for (int i = 0 ; i < dispatchee_count ; ++i)
    {
        canary_tomb.push_back(std::make_shared<mt::Signal>());
        auto current_canary = canary_tomb.back();
        auto canary = std::shared_ptr<int>(new int, [current_canary](int* victim) { delete victim; current_canary->raise(); });
        auto dispatchee = std::make_shared<mt::TestDispatchable>([canary]()
        {
            std::this_thread::sleep_for(std::chrono::seconds{1});
            EXPECT_THAT(canary.use_count(), Gt(0));
        });
        dispatcher->add_watch(dispatchee, md::DispatchReentrancy::reentrant);

        dispatchee->trigger();
    }

    for (auto& dispatchee : dispatchees)
    {
        dispatchee->trigger();
        dispatcher->remove_watch(dispatchee);
    }

    dispatchees.clear();
    dispatcher.reset();
    event_dispatcher.reset();

    for (auto headstone : canary_tomb)
    {
        // Use assert so as to not block for *ages* on failure
        ASSERT_TRUE(headstone->wait_for(std::chrono::seconds{2}));
    }
}

TEST(MultiplexingDispatchableTest, removes_dispatchable_that_returns_false_from_dispatch)
{
    bool dispatched{false};
    auto dispatchee = std::make_shared<mt::TestDispatchable>([&dispatched](md::FdEvents)
    {
        dispatched = true;
        return false;
    });
    md::MultiplexingDispatchable dispatcher{dispatchee};

    dispatchee->trigger();
    dispatchee->trigger();

    ASSERT_TRUE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_TRUE(dispatched);

    dispatched = false;
    while (mt::fd_is_readable(dispatcher.watch_fd()))
    {
        dispatcher.dispatch(md::FdEvent::readable);
    }

    EXPECT_FALSE(dispatched);
}

TEST(MultiplexingDispatchableTest, multiple_removals_are_threadsafe)
{
    using namespace testing;

    auto canary_killed = std::make_shared<mt::Signal>();
    auto canary = std::shared_ptr<int>(new int, [canary_killed](int* victim) { delete victim; canary_killed->raise(); });
    auto in_dispatch = std::make_shared<mt::Signal>();
    auto unblock_dispatchee = std::make_shared<mt::Signal>();

    auto dispatcher = std::make_shared<md::MultiplexingDispatchable>();

    auto first_dispatchee = std::make_shared<mt::TestDispatchable>([canary, in_dispatch, unblock_dispatchee]()
    {
        in_dispatch->raise();
        EXPECT_TRUE(unblock_dispatchee->wait_for(std::chrono::seconds{5}));
        EXPECT_THAT(canary.use_count(), Gt(0));
    });
    auto dummy_dispatchee = std::make_shared<mt::TestDispatchable>([](){});
    dispatcher->add_watch(first_dispatchee);
    dispatcher->add_watch(dummy_dispatchee);

    first_dispatchee->trigger();

    md::ThreadedDispatcher eventloop_one{"Bob", dispatcher};
    md::ThreadedDispatcher eventloop_two{"Gerry", dispatcher};

    EXPECT_TRUE(in_dispatch->wait_for(std::chrono::seconds{1}));

    dispatcher->remove_watch(dummy_dispatchee);
    dispatcher->remove_watch(first_dispatchee);
    dispatcher.reset();
    first_dispatchee.reset();
    dummy_dispatchee.reset();
    canary.reset();

    unblock_dispatchee->raise();

    EXPECT_TRUE(canary_killed->wait_for(std::chrono::seconds{2}));
}

TEST(MultiplexingDispatchableTest, automatic_removals_are_threadsafe)
{
    auto dispatcher = std::make_shared<md::MultiplexingDispatchable>();

    auto dispatchee = std::make_shared<mt::TestDispatchable>([](md::FdEvents) { return false; });

    dispatcher->add_watch(dispatchee, md::DispatchReentrancy::reentrant);

    md::ThreadedDispatcher eventloop{"Avocado", dispatcher};

    eventloop.add_thread();
    eventloop.add_thread();
    eventloop.add_thread();
    eventloop.add_thread();
    
    dispatchee->trigger();
}
