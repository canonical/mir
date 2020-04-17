/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/signal_blocker.h"
#include "mir/thread_name.h"

#include "mir/raii.h"
#include "mir/logging/logger.h"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <system_error>
#include <signal.h>
#include <boost/exception/all.hpp>
#include <algorithm>
#include <unordered_map>
#include <sys/eventfd.h>

namespace md = mir::dispatch;

class md::ThreadedDispatcher::ThreadShutdownRequestHandler : public md::Dispatchable
{
public:
    ThreadShutdownRequestHandler()
        : event_semaphore{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)},
          shutting_down{false}
    {
        if (event_semaphore == mir::Fd::invalid)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to create shutdown eventfd"}));
        }
    }

    mir::Fd watch_fd() const override
    {
        return event_semaphore;
    }

    bool dispatch(md::FdEvents events) override
    {
        if (events & md::FdEvent::error)
        {
            return false;
        }

        eventfd_t dummy;
        if (eventfd_read(event_semaphore, &dummy) < 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to clear shutdown notification"}));
        }
        std::lock_guard<decltype(running_flag_guard)> lock{running_flag_guard};
        *running_flags.at(std::this_thread::get_id()) = false;

        return true;
    }

    md::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }

    std::thread::id terminate_one_thread()
    {
        // First we tell a thread to die, any thread...
        if (eventfd_write(event_semaphore, 1) < 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to trigger thread shutdown"}));
        }

        // ...now we wait for a thread to die and tell us its ID...
        // We wait for a surprisingly long time because our threads are potentially blocked
        // in client code that we don't control.
        //
        // If the client is entirely unresponsive for a whole minute, it deserves to die.
        std::unique_lock<decltype(terminating_thread_mutex)> lock{terminating_thread_mutex};
        if (!thread_terminating.wait_for (lock,
                                          std::chrono::seconds{60},
                                          [this]() { return !terminating_threads.empty(); }))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Thread failed to shutdown"}));
        }

        auto killed_thread_id = terminating_threads.back();
        terminating_threads.pop_back();
        return killed_thread_id;
    }

    void terminate_all_threads_async()
    {
        eventfd_t thread_count;
        {
            std::lock_guard<std::mutex> lock(running_flag_guard);
            thread_count = running_flags.size();
            shutting_down = true;
        }
        if (eventfd_write(event_semaphore, thread_count) < 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to trigger thread shutdown"}));
        }
    }

    void register_thread(bool& run_flag)
    {
        std::lock_guard<decltype(running_flag_guard)> lock{running_flag_guard};

        running_flags[std::this_thread::get_id()] = &run_flag;
        if (shutting_down)
        {
            if (eventfd_write(event_semaphore, 1) < 0)
            {
                BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                         std::system_category(),
                                                         "Failed to trigger thread shutdown"}));
            }
        }
    }

    void unregister_thread()
    {
        {
            std::lock_guard<decltype(terminating_thread_mutex)> lock{terminating_thread_mutex};
            terminating_threads.push_back(std::this_thread::get_id());
        }
        thread_terminating.notify_one();
        {
            std::lock_guard<decltype(running_flag_guard)> lock{running_flag_guard};

            if (running_flags.erase(std::this_thread::get_id()) != 1)
            {
                BOOST_THROW_EXCEPTION((std::logic_error{"Attempted to unregister a not-registered thread"}));
            }
        }
    }

private:
    mir::Fd const event_semaphore;

    std::mutex terminating_thread_mutex;
    std::condition_variable thread_terminating;
    std::vector<std::thread::id> terminating_threads;

    std::mutex running_flag_guard;
    std::unordered_map<std::thread::id, bool*> running_flags;
    bool shutting_down;
};

namespace
{
void dispatch_loop(std::string const& name,
    std::shared_ptr<md::ThreadedDispatcher::ThreadShutdownRequestHandler> thread_register,
    std::shared_ptr<md::Dispatchable> dispatcher,
    std::function<void()> const& exception_handler)
{
    mir::set_thread_name(name);

    // This does not have to be std::atomic<bool> because thread_register is guaranteed to
    // only ever be dispatch()ed from one thread at a time.
    bool running{true};

    auto thread_registrar = mir::raii::paired_calls(
    [&running, thread_register]()
    {
        thread_register->register_thread(running);
    },
    [thread_register]()
    {
        thread_register->unregister_thread();
    });

    try
    {
        struct pollfd waiter;
        waiter.fd = dispatcher->watch_fd();
        waiter.events = POLL_IN;
        while (running)
        {
            if (poll(&waiter, 1, -1) < 0)
            {
                if (errno == EINTR)
                    continue;
                BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                         std::system_category(),
                                                         "Failed to wait for event"}));
            }
            dispatcher->dispatch(md::FdEvent::readable);
        }
    }
    catch(...)
    {
        exception_handler();
    }
}
}

md::ThreadedDispatcher::ThreadedDispatcher(std::string const& name, std::shared_ptr<Dispatchable> const& dispatchee)
    : ThreadedDispatcher(name, dispatchee, [](){ throw; })
{
}

md::ThreadedDispatcher::ThreadedDispatcher(std::string const& name,
                                           std::shared_ptr<md::Dispatchable> const& dispatchee,
                                           std::function<void()> const& exception_handler)
    : name_base{name},
      thread_exiter{std::make_shared<ThreadShutdownRequestHandler>()},
      dispatcher{std::make_shared<MultiplexingDispatchable>()},
      exception_handler{exception_handler}
{

    // We rely on exactly one thread at a time getting a shutdown message
    dispatcher->add_watch(thread_exiter, md::DispatchReentrancy::sequential);

    // But our target dispatchable is welcome to be dispatched on as many threads
    // as desired.
    dispatcher->add_watch(dispatchee, md::DispatchReentrancy::reentrant);

    mir::SignalBlocker blocker;
    threadpool.emplace_back(&dispatch_loop, name_base, thread_exiter, dispatcher, exception_handler);
}

md::ThreadedDispatcher::~ThreadedDispatcher() noexcept
{
    std::lock_guard<decltype(thread_pool_mutex)> lock{thread_pool_mutex};

    thread_exiter->terminate_all_threads_async();

    for (auto& thread : threadpool)
    {
        if (thread.get_id() == std::this_thread::get_id())
        {
            // We're being destroyed from within the dispatch callback
            // Attempting to join the eventloop will result in a trivial deadlock.
            //
            // The std::thread destructor will call std::terminate() for us, let's
            // leave a useful message.
            mir::logging::log(mir::logging::Severity::critical,
                              "Destroying ThreadedDispatcher from within a dispatch callback. This is a programming error.",
                              "Dispatch");
        }
        else
        {
            thread.join();
        }
    }
}

void md::ThreadedDispatcher::add_thread()
{
    std::lock_guard<decltype(thread_pool_mutex)> lock{thread_pool_mutex};
    mir::SignalBlocker blocker;
    threadpool.emplace_back(&dispatch_loop, name_base, thread_exiter, dispatcher, exception_handler);
}

void md::ThreadedDispatcher::remove_thread()
{
    auto terminated_thread_id = thread_exiter->terminate_one_thread();

    // Find that thread in our vector, join() it, then remove it.
    std::lock_guard<decltype(thread_pool_mutex)> threadpool_lock{thread_pool_mutex};

    auto dying_thread = std::find_if(threadpool.begin(),
                                     threadpool.end(),
                                     [&terminated_thread_id](std::thread const& candidate)
    {
            return candidate.get_id() == terminated_thread_id;
    });
    dying_thread->join();
    threadpool.erase(dying_thread);
}
