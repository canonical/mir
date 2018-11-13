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