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
 * Authored by: Andreasd Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/dispatch/action_queue.h"

#include "mir_test/fd_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace md = mir::dispatch;
using namespace ::testing;

TEST(ActionQueue, watch_fd_becomes_readable_on_enqueue)
{
    md::ActionQueue queue;

    ASSERT_FALSE(mt::fd_is_readable(queue.watch_fd()));
    queue.enqueue([]{});
    ASSERT_TRUE(mt::fd_is_readable(queue.watch_fd()));
}

TEST(ActionQueue, executes_action_on_dispatch)
{
    md::ActionQueue queue;

    auto action_exeuted = false;

    queue.enqueue([&](){action_exeuted = true;});
    ASSERT_FALSE(action_exeuted);
    queue.dispatch(md::FdEvent::readable);
    ASSERT_TRUE(action_exeuted);
}

TEST(ActionQueue, calls_nothing_on_error)
{
    md::ActionQueue queue;

    auto action_exeuted = false;

    queue.enqueue([&](){action_exeuted = true;});
    queue.dispatch(md::FdEvent::error);
    ASSERT_FALSE(action_exeuted);
}

TEST(ActionQueue, executes_action_only_once)
{
    md::ActionQueue queue;

    auto count_executed = 0;

    queue.enqueue([&](){++count_executed;});
    queue.dispatch(md::FdEvent::readable);
    EXPECT_THROW(queue.dispatch(md::FdEvent::readable),std::system_error);
    EXPECT_THROW(queue.dispatch(md::FdEvent::readable),std::system_error);
    EXPECT_THAT(count_executed, Eq(1));
}


