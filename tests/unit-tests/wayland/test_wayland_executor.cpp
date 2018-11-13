/*
 * Copyright © 2018 Canonical Ltd.
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

#include "src/server/frontend_wayland/wayland_executor.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <wayland-server-core.h>

#include "mir/test/fd_utils.h"
#include "mir/test/auto_unblock_thread.h"

namespace mt = mir::test;
namespace mf = mir::frontend;

using namespace testing;

MATCHER(FdIsReadable, "")
{
    return mt::fd_is_readable(mir::Fd{mir::IntOwnedFd{arg}});
}

class WaylandExecutorTest : public Test {
public:
    WaylandExecutorTest()
        : the_event_loop{wl_event_loop_create()},
          event_loop_fd{mir::IntOwnedFd{wl_event_loop_get_fd(the_event_loop)}}
    {
    }

    ~WaylandExecutorTest()
    {
        wl_event_loop_destroy(the_event_loop);
    }

    wl_event_loop* const the_event_loop;
    mir::Fd const event_loop_fd;
};

TEST_F(WaylandExecutorTest, spawning_a_task_makes_event_loop_fd_dispatchable)
{
    mf::WaylandExecutor executor{the_event_loop};

    EXPECT_THAT(event_loop_fd, Not(FdIsReadable()));

    executor.spawn([](){});

    EXPECT_THAT(event_loop_fd, FdIsReadable());
}

TEST_F(WaylandExecutorTest, dispatching_the_event_loop_dispatches_spawned_task)
{
    using namespace std::literals::chrono_literals;

    mf::WaylandExecutor executor{the_event_loop};

    bool executed{false};
    executor.spawn([&executed]() { executed = true; });

    while (mt::fd_is_readable(event_loop_fd))
    {
        wl_event_loop_dispatch(the_event_loop, 0);
        wl_event_loop_dispatch_idle(the_event_loop);
    }

    EXPECT_TRUE(executed);
}

TEST_F(WaylandExecutorTest, can_spawn_more_tasks_from_a_task)
{
    using namespace std::literals::chrono_literals;

    mf::WaylandExecutor executor{the_event_loop};

    bool inner_executed{false};
    executor.spawn(
        [&executor, &inner_executed]()
        {
            executor.spawn([&inner_executed]() { inner_executed = true; });
        });

    while (mt::fd_is_readable(event_loop_fd))
    {
        wl_event_loop_dispatch(the_event_loop, 0);
    }

    EXPECT_TRUE(inner_executed);
}

TEST_F(WaylandExecutorTest, can_destroy_exectuor_from_spawned_task)
{
    using namespace std::literals::chrono_literals;

    auto executor = new mf::WaylandExecutor{the_event_loop};

    bool executed{false};
    executor->spawn(
        [executor, &executed]()
        {
            executed = true;
            delete executor;
        });

    while (mt::fd_is_readable(event_loop_fd))
    {
        wl_event_loop_dispatch(the_event_loop, 0);
    }

    EXPECT_TRUE(executed);
}

TEST_F(WaylandExecutorTest, spawning_is_threadsafe)
{
    using namespace std::literals::chrono_literals;

    auto executor = std::make_shared<mf::WaylandExecutor>(the_event_loop);

    int const thread_count{100};
    int counter{0};
    std::vector<mt::AutoJoinThread> threads;

    // Spawn the threads from the wayland loop, to ensure it's started…
    executor->spawn(
        [executor, &threads, &counter]()
        {
            for (auto i = 0u ; i < thread_count ; ++i)
            {
                threads.emplace_back(
                    mt::AutoJoinThread{
                        [executor, &counter]()
                        {
                            // These calls should all be serialised onto the wayland loop,
                            // so no need for atomics or mutexes…
                            executor->spawn([&counter]() { ++counter; });
                        }});
            }
        });

    while (mt::fd_becomes_readable(event_loop_fd, 1s))
    {
        wl_event_loop_dispatch(the_event_loop, 0);
    }

    EXPECT_THAT(counter, Eq(thread_count));
}
