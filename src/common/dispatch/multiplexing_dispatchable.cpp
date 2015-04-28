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

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "utils.h"
#include "mir/raii.h"

#include <boost/throw_exception.hpp>

#include <sys/epoll.h>
#include <poll.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <system_error>
#include <algorithm>

namespace md = mir::dispatch;

namespace
{
class DispatchableAdaptor : public md::Dispatchable
{
public:
    DispatchableAdaptor(mir::Fd const& fd, std::function<void()> const& callback)
        : fd{fd},
          handler{callback}
    {
    }

    mir::Fd watch_fd() const override
    {
        return fd;
    }

    bool dispatch(md::FdEvents events) override
    {
        if (events & md::FdEvent::error)
        {
            return false;
        }
        handler();
        return true;
    }

    md::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }
private:
    mir::Fd const fd;
    std::function<void()> const handler;
};

class ReadLock
{
public:
    ReadLock(pthread_rwlock_t& lock)
        : mutex{&lock}
    {
        auto err = pthread_rwlock_rdlock(mutex);
        if (err != 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{err,
                                                     std::system_category(),
                                                     "Failed to acquire read lock"}));
        }
    }

    ~ReadLock() noexcept
    {
        pthread_rwlock_unlock(mutex);
    }
private:
    pthread_rwlock_t* mutex;
};

class WriteLock
{
public:
    WriteLock(pthread_rwlock_t& lock)
        : mutex{&lock}
    {
        auto err = pthread_rwlock_wrlock(mutex);
        if (err != 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{err,
                                                     std::system_category(),
                                                     "Failed to acquire write lock"}));
        }
    }

    ~WriteLock() noexcept
    {
        pthread_rwlock_unlock(mutex);
    }
private:
    pthread_rwlock_t* mutex;
};
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

    pthread_rwlockattr_t attr;
    int err;
    err = pthread_rwlockattr_init(&attr);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{err,
                                                 std::system_category(),
                                                 "Failed to init pthread attrs"}));
    }
    // Set writer preference; otherwise remove_watch could block indefinitely
    err = pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{err,
                                                 std::system_category(),
                                                 "Failed to set preferred rw-lock mode"}));
    }
    err = pthread_rwlock_init(&lifetime_mutex, &attr);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{err,
                                                 std::system_category(),
                                                 "Failed to init rw-lock"}));
    }

    pthread_rwlockattr_destroy(&attr);
}

md::MultiplexingDispatchable::~MultiplexingDispatchable() noexcept
{
    pthread_rwlock_destroy(&lifetime_mutex);
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

    std::shared_ptr<md::Dispatchable> source;
    bool rearm_source{false};
    epoll_event event;

    {
        ReadLock lock{lifetime_mutex};

        auto result = epoll_wait(epoll_fd, &event, 1, 0);

        if (result < 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to wait on fds"}));
        }

        if (result == 0)
        {
            // Some other thread must have stolen the event we were woken for;
            // that's ok, just return.
            return true;
        }

        auto event_source = reinterpret_cast<decltype(dispatchee_holder)::pointer>(event.data.ptr);

        source = event_source->first;
        rearm_source = event_source->second;
    }

    if (!source->dispatch(epoll_to_fd_event(event)))
    {
        remove_watch(source);
    }
    else if (rearm_source)
    {
        event.events = fd_event_to_epoll(source->relevant_events()) | EPOLLONESHOT;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, source->watch_fd(), &event);
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
        WriteLock lock{lifetime_mutex};
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
        WriteLock lock{lifetime_mutex};
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

void md::MultiplexingDispatchable::add_watch(Fd const& fd, std::function<void()> const& callback)
{
    add_watch(std::make_shared<DispatchableAdaptor>(fd, callback));
}

void md::MultiplexingDispatchable::remove_watch(std::shared_ptr<Dispatchable> const& dispatchee)
{
    remove_watch(dispatchee->watch_fd());
}

void md::MultiplexingDispatchable::remove_watch(Fd const& fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr))
    {
        if (errno == ENOENT)
        {
            // If reentrant dispatch returns false we can try to remove the same dispatchable twice.
            //
            // The reference-counting on mir::Fd should prevent the fd being closed, and
            // hence the handle being reused, before we've processed all such removals,
            // so this should not be racy with new Dispatchable creation + add_watch.
            return;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to remove fd monitor"}));
    }

    WriteLock lock{lifetime_mutex};
    dispatchee_holder.remove_if([&fd](std::pair<std::shared_ptr<Dispatchable>,bool> const& candidate)
    {
        return candidate.first->watch_fd() == fd;
    });
}
