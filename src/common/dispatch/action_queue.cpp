/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/dispatch/action_queue.h"

#include <boost/throw_exception.hpp>

mir::dispatch::ActionQueue::ActionQueue()
{
    int pipefds[2];
    if (pipe(pipefds) < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create notification pipe for IO"}));
    }
    read_fd = mir::Fd{pipefds[0]};
    trigger_fd = mir::Fd{pipefds[1]};
}

mir::Fd mir::dispatch::ActionQueue::watch_fd() const
{
    return read_fd;
}

void mir::dispatch::ActionQueue::enqueue(std::function<void()> const& action)
{
    {
        std::unique_lock<std::mutex> lock(list_lock);
        actions.push_back(action);
    }
    wake();
}

bool mir::dispatch::ActionQueue::dispatch(FdEvents events)
{
    if (events&FdEvent::error)
        return false;

    std::list<std::function<void()>> actions_to_process;

    {
        std::unique_lock<std::mutex> lock(list_lock);
        std::swap(actions_to_process, actions);
    }

    while(!actions_to_process.empty())
    {
        consume();
        actions_to_process.front()();
        actions_to_process.pop_front();
    }
    return true;
}

mir::dispatch::FdEvents mir::dispatch::ActionQueue::relevant_events() const
{
    return FdEvent::readable|FdEvent::error;
}

void mir::dispatch::ActionQueue::consume()
{
    uint64_t dummy;
    if (read(read_fd, &dummy, sizeof(dummy)) != sizeof(dummy))
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to consume action queue notification"}));
    }
}

void mir::dispatch::ActionQueue::wake()
{
    uint64_t dummy{0};
    if (write(trigger_fd, &dummy, sizeof(dummy)) != sizeof(dummy))
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to wake action queue"}));
    }
}
