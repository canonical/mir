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

template<typename VictimReference>
void add_to_gc_queue(mir::Fd const& queue,
                     std::atomic<int>* generation,
                     VictimReference victim,
                     std::weak_ptr<md::Dispatchable>* predecessor)
{
    constexpr ssize_t gc_data_size{sizeof(generation) + sizeof(victim) + sizeof(predecessor)};
    static_assert(gc_data_size < PIPE_BUF,
                  "Size of data for delayed GC must be less than PIPE_BUF for atomic guarantees");

    std::unique_ptr<char[]> gc_data{new char[gc_data_size]};

    memcpy(gc_data.get(), &generation, sizeof(generation));
    memcpy(gc_data.get() + sizeof(generation), &victim, sizeof(victim));
    memcpy(gc_data.get() + sizeof(generation) + sizeof(victim), &predecessor, sizeof(predecessor));

    // Loop to protect against interruption by signals
    while (write(queue, gc_data.get(), gc_data_size) < gc_data_size)
    {
        if (errno != EINTR)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to write to delayed GC queue"}));
        }
    }
}

template<typename VictimReference>
void pull_from_gc_queue(mir::Fd const& queue,
                        std::atomic<int>*& generation,
                        VictimReference& victim,
                        std::weak_ptr<md::Dispatchable>*& predecessor)
{
    static_assert((sizeof(generation) + sizeof(victim) + sizeof(predecessor)) < PIPE_BUF,
                  "Size of data for delayed GC must be less than PIPE_BUF for atomic guarantees");

    // The PIPE_BUF check above guarantees that we won't get partial reads,
    // so the only possible error we can recover from is EINTR, and we can
    // simply retry.
    while (read(queue, &generation, sizeof(generation)) != sizeof(generation))
    {
        if (errno != EINTR)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to read from delayed GC queue"}));
        }
    }
    while (read(queue, &victim, sizeof(victim)) != sizeof(victim))
    {
        if (errno != EINTR)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to read from delayed GC queue"}));
        }
    }
    while (read(queue, &predecessor, sizeof(predecessor)) != sizeof(predecessor))
    {
        if (errno != EINTR)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to read from delayed GC queue"}));
        }
    }
}
}

md::MultiplexingDispatchable::MultiplexingDispatchable()
    : in_current_generation{new std::atomic<int>{0}},
      last_gc_victim{new std::weak_ptr<Dispatchable>{}},
      epoll_fd{mir::Fd{::epoll_create1(EPOLL_CLOEXEC)}}
{
    if (epoll_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create epoll monitor"}));
    }

    int pipefds[2];
    if (pipe(pipefds) < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create delayed-destroy pipe"}));
    }

    /*
     * Delayed GC queue implementation
     */
    gc_queue = mir::Fd{pipefds[1]};
    gc_read_queue = mir::Fd{pipefds[0]};
    auto gc_dispatchable = std::make_shared<DispatchableAdaptor>(gc_read_queue,
                                                                 [this]()
    {
        std::atomic<int>* generation;
        decltype(dispatchee_holder)::const_iterator victim;
        std::weak_ptr<Dispatchable>* predecessor;

        pull_from_gc_queue(gc_read_queue, generation, victim, predecessor);

        if (*generation == 0 && predecessor->expired())
        {
            /*
             * All of the threads since our predecessor was queued for destruction have
             * exited dispatch(), and our predecessor has been destroyed.
             *
             * By induction, all threads that might be currently dispatch()ing us have
             * exited dispatch(). It is now safe to destroy.
             */
            std::lock_guard<decltype(lifetime_mutex)> lock{lifetime_mutex};
            dispatchee_holder.erase(victim);
            delete generation;
            delete predecessor;
        }
        else
        {
            add_to_gc_queue(gc_queue, generation, victim, predecessor);
        }
    });

    add_watch(gc_dispatchable, DispatchReentrancy::sequential);
}

md::MultiplexingDispatchable::~MultiplexingDispatchable() noexcept
{
    delete in_current_generation.load();
    delete last_gc_victim;

    /*
     * The dispatchee_holder destructor will clean up all the Dispatchables, but
     * if we have any pending GC queued up we need to free the generation atomics.
     */
    pollfd pending_gc;
    pending_gc.events = POLLIN;
    pending_gc.fd = gc_read_queue;

    while (poll(&pending_gc, 1, 0) > 0)
    {
        std::atomic<int>* generation;
        decltype(dispatchee_holder)::const_iterator victim;
        std::weak_ptr<Dispatchable>* predecessor;

        pull_from_gc_queue(gc_read_queue, generation, victim, predecessor);
        delete generation;
        delete predecessor;
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

    std::atomic<int>* our_generation = in_current_generation.load();
    auto count_handler = mir::raii::paired_calls([our_generation]() { ++(*our_generation); },
                                                 [our_generation]() { --(*our_generation); });

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

        if (!event_source->first->dispatch(epoll_to_fd_event(event)))
        {
            remove_watch(event_source->first);
        }

        if (event_source->second)
        {
            event.events = fd_event_to_epoll(event_source->first->relevant_events()) | EPOLLONESHOT;
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
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to remove fd monitor"}));
    }

    /*
     * Theory of operation:
     * The kernel guarantees that any call to epoll_wait at this point will not return an
     * event from the fd we've just removed. That means all we need to do to safely destroy
     * the Dispatchable is wait for all the dispatch()es that are currently running to finish.
     *
     * If we know it's safe, we delete immediately. Otherwise, we push the data needed to detect
     * if it's safe onto the GC queue and rely on that.
     *
     * The two things we know:
     * 1) in_current_generation is the number of threads that have entered dispatch() since the last
     *    call to remove_request() that have not yet left.
     * 2) If last_gc_victim->expired() is true then there are no pending remove requests.
     */
    {
        std::lock_guard<decltype(lifetime_mutex)> lock{lifetime_mutex};

        decltype(dispatchee_holder)::const_iterator victim;
        victim = std::find_if(dispatchee_holder.begin(), dispatchee_holder.end(),
                              [&fd](std::pair<std::shared_ptr<Dispatchable>,bool> const& candidate)
        {
            return candidate.first->watch_fd() == fd;
        });

        if (*in_current_generation == 0 && last_gc_victim->expired())
        {
            /*
             * in_current_generation == 0 means that no threads started after the last remove_watch
             * are in dispatch().
             *
             * last_gc_victim->expired() means that any threads possibly in dispatch() when the last
             * remove_watch call was made have left.
             *
             * We can therefore free immediately.
             */

            dispatchee_holder.erase(victim);
        }
        else
        {
            /*
             * Either there are threads currently in dispatch() or we're not sure that all threads
             * from a previous remove_watch() have left dispatch().
             *
             * Save the current generation and replace it so that new threads increment a different counter.
             * This means that current_generation becomes strictly decreasing and hits 0 when the last
             * thread currently in dispatch() leaves it.
             */
            std::atomic<int>* current_generation = in_current_generation.load();
            auto next_generation = new std::atomic<int>{0};
            in_current_generation = next_generation;

            add_to_gc_queue(gc_queue, current_generation, victim, last_gc_victim);

            last_gc_victim = new std::weak_ptr<Dispatchable>{victim->first};
        }
    }
}
