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
#include "mir/logging/logger.h"
#include "utils.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <system_error>
#include <array>
#include <signal.h>
#include <boost/exception/all.hpp>

namespace md = mir::dispatch;

namespace
{

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
    event.events = md::fd_event_to_epoll(dispatchee->relevant_events());
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dispatchee->watch_fd(), &event);

    for (;;)
    {
        std::array<epoll_event,2> events;
        auto const num_available_events =
            epoll_wait(epoll_fd, events.data(), events.size(), -1);

        if (num_available_events == 1)
        {
            if (events[0].data.u32 == fd_names::dispatchee_fd)
            {
                if (!dispatchee->dispatch(md::epoll_to_fd_event(events[0])))
                {
                    // No need to keep looping, the Dispatchable's not going to produce any more events.
                    return;
                }
            }
            else if (events[0].data.u32 == fd_names::shutdown)
            {
                // The only thing we do with the shutdown fd is to close it.
                return;
            }
        }
        else if (num_available_events > 1)
        {
            // Because we only have two fds in the epoll, if there is more than one
            // event pending then one of them must be a shutdown event.
            // So, shutdown.
            return;
        }
        else if (num_available_events < 0)
        {
            // Although we have blocked signals in this thread, we can still
            // get interrupted by SIGSTOP (which is unblockable and non-fatal).
            if (errno != EINTR)
            {
                BOOST_THROW_EXCEPTION((std::system_error{
                    errno, std::system_category(), "Failed to wait for epoll events"}));
            }
        }
    }
}

}

md::SimpleDispatchThread::SimpleDispatchThread(std::shared_ptr<md::Dispatchable> const& dispatchee)
    : SimpleDispatchThread(dispatchee, []{})
{}

md::SimpleDispatchThread::SimpleDispatchThread(
    std::shared_ptr<md::Dispatchable> const& dispatchee,
    std::function<void()> const& exception_handler)
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
    eventloop = std::thread{
        [exception_handler, dispatchee, terminate_fd]()
        {
            // Our IO threads must not receive any signals
            sigset_t all_signals;
            sigfillset(&all_signals);

            if (auto error = pthread_sigmask(SIG_BLOCK, &all_signals, NULL))
                BOOST_THROW_EXCEPTION((
                    std::system_error{error,
                        std::system_category(),
                        "Failed to block signals on IO thread"}));

            try
            {
                wait_for_events_forever(dispatchee, terminate_fd);
            }
            catch(...)
            {
                exception_handler();
            }
        }};
}

md::SimpleDispatchThread::~SimpleDispatchThread() noexcept
{
    shutdown_fd = mir::Fd{};
    if (eventloop.get_id() == std::this_thread::get_id())
    {
        // We're being destroyed from within the dispatch callback
        // Attempting to join the eventloop will result in a trivial deadlock.
        // 
        // The std::thread destructor will call std::terminate() for us, let's
        // leave a useful message.
        mir::logging::log(mir::logging::Severity::critical,
                          "Destroying SimpleDispatchThread from within a dispatch callback. This is a programming error.",
                          "Dispatch");
    }
    else if (eventloop.joinable())
    {
        eventloop.join();
    }
}
