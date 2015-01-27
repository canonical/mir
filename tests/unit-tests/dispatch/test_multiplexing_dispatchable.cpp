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

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/fd.h"
#include "mir_test/pipe.h"
#include "mir_test/signal.h"
#include "mir_test/fd_utils.h"
#include "mir_test/test_dispatchable.h"

#include <fcntl.h>

#include <atomic>

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

    dispatchable->trigger();

    EXPECT_FALSE(mt::fd_is_readable(dispatcher.watch_fd()));
    dispatcher.dispatch(md::FdEvent::readable);

    EXPECT_FALSE(dispatched);
}
