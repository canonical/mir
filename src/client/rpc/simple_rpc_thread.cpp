/*
 * Copyright © 2014 Canonical Ltd.
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

#include "simple_rpc_thread.h"
#include "dispatchable.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <system_error>
#include <signal.h>
#include <boost/exception/all.hpp>

namespace mclr = mir::client::rpc;

namespace
{
void wait_for_events_forever(std::shared_ptr<mclr::Dispatchable> const& dispatchee, mir::Fd shutdown_fd)
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

    // We only care when the shutdown pipe has been closed
    event.events = EPOLLRDHUP;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, shutdown_fd, &event);

    // We want readability or closure for the dispatchee.
    event.events = EPOLLIN | EPOLLRDHUP;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dispatchee->watch_fd(), &event);

    for (;;)
    {
        epoll_wait(epoll_fd, &event, 1, -1);
        if (event.events & EPOLLIN)
        {
            dispatchee->dispatch();
        }
        else if (event.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
        {
            // On hangup we go away.
            return;
        }
    }
}

}

mclr::SimpleRpcThread::SimpleRpcThread(std::shared_ptr<mclr::Dispatchable> const& dispatchee)
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

mclr::SimpleRpcThread::~SimpleRpcThread() noexcept
{
    ::close(shutdown_fd);
    if (eventloop.joinable())
    {
        eventloop.join();
    }
}
