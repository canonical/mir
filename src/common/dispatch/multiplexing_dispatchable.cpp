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

#include "mir/dispatch/multiplexing_dispatchable.h"

#include <boost/throw_exception.hpp>

#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <system_error>

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
}

md::MultiplexingDispatchable::MultiplexingDispatchable()
    : epoll_fd{mir::Fd{::epoll_create1(EPOLL_CLOEXEC)}}
{
    if (epoll_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create epoll monitor"}));
    }
}

md::MultiplexingDispatchable::MultiplexingDispatchable(std::initializer_list<std::shared_ptr<Dispatchable>> dispatchees)
    : MultiplexingDispatchable()
{
    for (auto& target : dispatchees)
    {
        add_watch(target);
    }
}

mir::Fd md::MultiplexingDispatchable::watch_fd() const
{
    return epoll_fd;
}

bool md::MultiplexingDispatchable::dispatch(md::FdEvents events)
{
    if (events & md::FdEvent::error)
    {
        return false;
    }

    epoll_event event;

    auto result = epoll_wait(epoll_fd, &event, 1, 0);

    if (result < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to wait on fds"}));
    }

    if (result > 0)
    {
        auto event_source = reinterpret_cast<std::pair<std::shared_ptr<Dispatchable>, bool>*>(event.data.ptr);

        event_source->first->dispatch(epoll_to_fd_event(event));

        if (event_source->second)
        {
            event.events = event_source->first->relevant_events() | EPOLLONESHOT;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_source->first->watch_fd(), &event);
        }
    }
    return true;
}

md::FdEvents md::MultiplexingDispatchable::relevant_events() const
{
    return md::FdEvent::readable;
}

void md::MultiplexingDispatchable::add_watch(std::shared_ptr<md::Dispatchable> const& dispatchee)
{
    add_watch(dispatchee, DispatchReentrancy::sequential);
}

void md::MultiplexingDispatchable::add_watch(std::shared_ptr<md::Dispatchable> const& dispatchee,
                                             DispatchReentrancy reentrancy)
{
    decltype(dispatchee_holder)::iterator new_holder;
    {
        std::lock_guard<decltype(lifetime_mutex)> lock{lifetime_mutex};
        new_holder = dispatchee_holder.emplace(dispatchee_holder.begin(),
                                               dispatchee,
                                               reentrancy == DispatchReentrancy::sequential);
    }

    epoll_event e;
    ::memset(&e, 0, sizeof(e));

    e.events = fd_event_to_epoll(dispatchee->relevant_events());
    if (reentrancy == DispatchReentrancy::sequential)
    {
        e.events |= EPOLLONESHOT;
    }
    e.data.ptr = static_cast<void*>(&(*new_holder));
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dispatchee->watch_fd(), &e) < 0)
    {
        std::lock_guard<decltype(lifetime_mutex)> lock{lifetime_mutex};
        dispatchee_holder.erase(new_holder);
        if (errno == EEXIST)
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"Attempted to monitor the same fd twice"}));
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to monitor fd"}));
    }
}

void md::MultiplexingDispatchable::remove_watch(std::shared_ptr<Dispatchable> const& dispatchee)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, dispatchee->watch_fd(), nullptr))
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to remove fd monitor"}));
    }

    {
        std::lock_guard<decltype(lifetime_mutex)> lock{lifetime_mutex};
        dispatchee_holder.remove_if ([&dispatchee](std::pair<std::shared_ptr<Dispatchable>,bool> const& candidate)
        {
            return candidate.first->watch_fd() == dispatchee->watch_fd();
        });
    }
}
