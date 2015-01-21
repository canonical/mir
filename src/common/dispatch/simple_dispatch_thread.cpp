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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/dispatch/simple_dispatch_thread.h"
#include "mir/dispatch/dispatchable.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <system_error>
#include <signal.h>
#include <boost/exception/all.hpp>

namespace md = mir::dispatch;

namespace
{
md::FdEvents epoll_to_fd_event(epoll_event const& event)
{
    md::FdEvents val{0};
    if (event.events & EPOLLIN)
    {
        val |= md::FdEvent::readable;
    }
    if (event.events & EPOLLOUT)
    {
        val = md::FdEvent::writable;
    }
    if (event.events & (EPOLLHUP | EPOLLRDHUP))
    {
        val |= md::FdEvent::remote_closed;
    }
    if (event.events & EPOLLERR)
    {
        val = md::FdEvent::error;
    }
    return val;
}

int fd_event_to_epoll(md::FdEvents const& event)
{
    int epoll_value{0};
    if (event & md::FdEvent::readable)
    {
        epoll_value |= EPOLLIN;
    }
    if (event & md::FdEvent::writable)
    {
        epoll_value |= EPOLLOUT;
    }
    if (event & md::FdEvent::remote_closed)
    {
        epoll_value |= EPOLLRDHUP | EPOLLHUP;
    }
    if (event & md::FdEvent::error)
    {
        epoll_value |= EPOLLERR;
    }
    return epoll_value;
}

void wait_for_events_forever(std::shared_ptr<md::Dispatchable> const& dispatchee, mir::Fd shutdown_fd)
{
    auto epoll_fd = mir::Fd{epoll_create1(0)};
    if (epoll_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create epoll IO monitor"}));
    }
    epoll_event event;
    memset(&event, 0, sizeof(event));

    enum fd_names : uint32_t {
        shutdown,
        dispatchee_fd
    };

    // We only care when the shutdown pipe has been closed
    event.data.u32 = fd_names::shutdown;
    event.events = EPOLLRDHUP;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, shutdown_fd, &event);

    // Ask the dispatchee what it events it's interested in...
    event.data.u32 = fd_names::dispatchee_fd;
    event.events = fd_event_to_epoll(dispatchee->relevant_events());
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dispatchee->watch_fd(), &event);

    for (;;)
    {
        epoll_wait(epoll_fd, &event, 1, -1);
        if (event.data.u32 == fd_names::dispatchee_fd)
        {
            if (!dispatchee->dispatch(epoll_to_fd_event(event)))
            {
                // No need to keep looping, the Dispatchable's not going to produce any more events.
                return;
            }
        }
        else if (event.data.u32 == fd_names::shutdown)
        {
            // The only thing we do with the shutdown fd is to close it.
            return;
        }
    }
}

}

md::SimpleDispatchThread::SimpleDispatchThread(std::shared_ptr<md::Dispatchable> const& dispatchee)
{
    int pipefds[2];
    if (pipe(pipefds) < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create shutdown pipe for IO thread"}));
    }
    shutdown_fd = mir::Fd{pipefds[1]};
    mir::Fd const terminate_fd = mir::Fd{pipefds[0]};
    eventloop = std::thread{[dispatchee, terminate_fd]()
                            {
                                // Our IO threads must not receive any signals
                                sigset_t all_signals;
                                sigfillset(&all_signals);

                                if (auto error = pthread_sigmask(SIG_BLOCK, &all_signals, NULL))
                                    BOOST_THROW_EXCEPTION((
                                                std::system_error{error,
                                                                  std::system_category(),
                                                                  "Failed to block signals on IO thread"}));

                                wait_for_events_forever(dispatchee, terminate_fd);
                            }};
}

md::SimpleDispatchThread::~SimpleDispatchThread() noexcept
{
    shutdown_fd = mir::Fd{};
    if (eventloop.joinable())
    {
        eventloop.join();
    }
}
