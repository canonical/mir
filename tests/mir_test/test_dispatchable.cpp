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

#include <fcntl.h>

#include "mir/test/pipe.h"
#include "mir/test/test_dispatchable.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace md = mir::dispatch;

mt::TestDispatchable::TestDispatchable(std::function<bool(md::FdEvents)> const& target,
                                       md::FdEvents relevant_events)
    : target{target},
      eventmask{relevant_events}
{
    // Need to use O_NONBLOCK here to ensure concurrent dispatch doesn't block indefinitely
    // in read()
    mt::Pipe pipe{O_NONBLOCK};
    read_fd = pipe.read_fd();
    write_fd = pipe.write_fd();
}

mt::TestDispatchable::TestDispatchable(std::function<bool(md::FdEvents)> const& target)
    : TestDispatchable(target, md::FdEvent::readable)
{
}

mt::TestDispatchable::TestDispatchable(std::function<void()> const& target)
    : TestDispatchable([target](md::FdEvents) { target(); return true; })
{
}

mir::Fd mt::TestDispatchable::watch_fd() const
{
    return read_fd;
}

bool mt::TestDispatchable::dispatch(md::FdEvents events)
{
    auto continue_dispatch = target(events);
    if (!(events & md::FdEvent::remote_closed))
    {
        // There's no way to untrigger remote hangup :)
        untrigger();
    }
    return continue_dispatch;
}

md::FdEvents mt::TestDispatchable::relevant_events() const
{
    return eventmask;
}

void mt::TestDispatchable::trigger()
{
    using namespace testing;
    char dummy{0};
    EXPECT_THAT(::write(write_fd, &dummy, sizeof(dummy)), Eq(sizeof(dummy)));
}

void mt::TestDispatchable::untrigger()
{
    using namespace testing;
    char dummy{0};
    auto val = ::read(read_fd, &dummy, sizeof(dummy));
    if (val < 0)
    {
        EXPECT_THAT(errno, Eq(EAGAIN));
    }
    else
    {
        EXPECT_THAT(val, Eq(sizeof(dummy)));
    }
}

void mt::TestDispatchable::hangup()
{
    write_fd = mir::Fd{};
}
