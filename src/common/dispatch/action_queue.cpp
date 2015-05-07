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
#include <sys/eventfd.h>

mir::dispatch::ActionQueue::ActionQueue()
    : event_fd{eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK|EFD_SEMAPHORE)}
{
    if (event_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create event fd for action queue"}));
}

mir::Fd mir::dispatch::ActionQueue::watch_fd() const
{
    return event_fd;
}

void mir::dispatch::ActionQueue::enqueue(std::function<void()> const& action)
{
    std::unique_lock<std::mutex> lock(list_lock);
    actions.push_back(action);
    wake();
}

bool mir::dispatch::ActionQueue::dispatch(FdEvents events)
{
    if (events&FdEvent::error)
        return false;

    if (!consume())
    {
        // Another thread has won the race to process this event
        // That's OK, just bail.
        return true;
    }

    std::function<void()> action_to_process;

    {
        std::unique_lock<std::mutex> lock(list_lock);
        action_to_process = actions.front();
        actions.pop_front();
    }

    action_to_process();

    return true;
}

mir::dispatch::FdEvents mir::dispatch::ActionQueue::relevant_events() const
{
    return FdEvent::readable;
}

bool mir::dispatch::ActionQueue::consume()
{
    uint64_t num_actions;
    if (read(event_fd, &num_actions, sizeof num_actions) != sizeof num_actions)
    {
        if (errno == EAGAIN)
        {
            return false;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to consume action queue notification"}));
    }
    return true;
}

void mir::dispatch::ActionQueue::wake()
{
    uint64_t one_more{1};
    if (write(event_fd, &one_more, sizeof one_more) != sizeof one_more)
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to wake action queue"}));
}
