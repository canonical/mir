/*
 * Copyright © 2015-2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "wayland_executor.h"

#include "mir/fd.h"
#include "mir/log.h"

#include <sys/eventfd.h>

#include <boost/throw_exception.hpp>

#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <system_error>

namespace mf = mir::frontend;

/*
 * Theory of execution:
 *
 * Like many interactions with Wayland API we have the problem that we can only
 * call Wayland functions on the mainloop thread *and* that the Wayland objects
 * can be destroyed from underneath the wrappers.
 *
 * The solution we use here is to share ownership of the workqueue between the
 * wl_event_source and the WaylandExecutor. WaylandExecutor can then always
 * enqueue new work, even if no more work is going to be processed, and the work
 * processing function always has a reference to the workqueue state.
 */

class mf::WaylandExecutor::State
{
private:
    enum class ExecutionState
    {
        Running,
        TerminationRequested,
        Stopped
    };
public:
    explicit State(wl_event_loop* loop)
        : loop{loop}
    {
    }

    void enqueue(std::function<void()>&& work)
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (state == ExecutionState::Running)
        {
            workqueue.emplace_back(std::move(work));
        }
        // If we've been terminated then drop the work on the floor, letting the
        // std::function destructor clean up any necessary state.
    }

    void enqueue_termination(std::function<void()>&& terminator)
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (state == ExecutionState::Running)
        {
            workqueue.emplace_front(std::move(terminator));
            state = ExecutionState::TerminationRequested;
        }
    }

    std::function<void()> get_work()
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (!workqueue.empty())
        {
            auto const work = std::move(workqueue.front());
            workqueue.pop_front();
            return work;
        }
        return {};
    }

    std::unique_lock<std::mutex> drain()
    {
        std::unique_lock<std::mutex> lock{mutex};

        if (state == ExecutionState::TerminationRequested)
        {
            // If we've been asked to terminate then the front of the workqueue
            // will contain the termination request.
            {
                std::function<void()> const work = std::move(workqueue.front());
                lock.unlock();

                work();
            }
            lock.lock();
        }

        state = ExecutionState::Stopped;
        workqueue.empty();

        return lock;
    }

    static int on_notify(int fd, uint32_t, void* data);
private:
    std::mutex mutex;
    ExecutionState state{ExecutionState::Running};
    wl_event_loop* const loop;
    std::deque<std::function<void()>> workqueue;
};

namespace
{
class EventLoopDestroyedHandler
{
public:
    static void setup_destruction_handler_for_loop(
        wl_event_loop* loop,
        wl_event_source* source,
        std::shared_ptr<mf::WaylandExecutor::State> state)
    {
        new EventLoopDestroyedHandler(loop, source, std::move(state));
    }

    static void remove_destruction_handler_for_loop(
        wl_event_loop* loop)
    {
        auto listener =
            wl_event_loop_get_destroy_listener(loop, &EventLoopDestroyedHandler::on_destroyed);

        EventLoopDestroyedHandler* me;
        me = wl_container_of(listener, me, destruction_listener);
        delete me;

        wl_list_remove(&listener->link);
    }
private:
    EventLoopDestroyedHandler(
        wl_event_loop* loop,
        wl_event_source* source,
        std::shared_ptr<mf::WaylandExecutor::State> state)
        : source{source},
          work_queue{std::move(state)}
    {
        destruction_listener.notify = &EventLoopDestroyedHandler::on_destroyed;
        wl_event_loop_add_destroy_listener(loop, &destruction_listener);
    }

    static void on_destroyed(wl_listener* listener, void*)
    {
        EventLoopDestroyedHandler* me;

        me = wl_container_of(listener, me, destruction_listener);

        {
            /*
             * We drain the work queue to ensure that any shutdown work has a chance
             * to be processed.
             */
            auto queue_lock = me->work_queue->drain();

            /*
             * We take no weak pointers to the work_queue, so if it's unique now it's
             * guaranteed to remain unique. We've got a lock on the queue, so
             * ~WaylandExecutor can't sneakily add a cleanup work item we would never
             * process.
             *
             * If work_queue *is* unique that means that ~WaylandExecutor has already
             * unregistered the event source, so we should not try to do so again.
             */
            if (!me->work_queue.unique())
            {
                wl_event_source_remove(me->source);
            }
        }

        delete me;
    }

    wl_event_source* const source;
    std::shared_ptr<mf::WaylandExecutor::State> const work_queue;

    wl_listener destruction_listener;
};

static_assert(
    std::is_standard_layout<EventLoopDestroyedHandler>::value,
    "DestructionShim must be Standard Layout for wl_container_of to be defined behaviour");
}

int mf::WaylandExecutor::State::on_notify(int fd, uint32_t, void* data)
{
    auto state = static_cast<State*>(data);

    eventfd_t unused;
    if (auto err = eventfd_read(fd, &unused))
    {
        mir::log_error(
            "eventfd_read failed to consume wakeup notification: %s (%i)",
            strerror(err),
            err);
    }

    while (auto work = state->get_work())
    {
        try
        {
            work();
        }
        catch (...)
        {
            mir::log(
                mir::logging::Severity::critical,
                MIR_LOG_COMPONENT,
                std::current_exception(),
                "Exception processing Wayland event loop work item");
        }
    }
    if (state->state != ExecutionState::Running)
    {
        EventLoopDestroyedHandler::remove_destruction_handler_for_loop(state->loop);
    }

    return 0;
}



mf::WaylandExecutor::WaylandExecutor(wl_event_loop* loop)
    : state{std::make_shared<State>(loop)},
      notify_fd{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK)},
      source{wl_event_loop_add_fd(
          loop,
          notify_fd,
          WL_EVENT_READABLE,
          &State::on_notify,
          state.get())}
{
    if (notify_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to create IPC pause notification eventfd"}));
    }
    if (!source)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to create Wayland notify source"}));
    }
    EventLoopDestroyedHandler::setup_destruction_handler_for_loop(loop, source, state);
}

mf::WaylandExecutor::~WaylandExecutor()
{
    /*
     * We need to move state into the functor; the destruction machinery relies
     * on WaylandExecutor *not* having a ref on state to determine whether or not
     * to unregister the source itself.
     *
     * If we enqueue the work before dropping WaylandExecutors' ref there's a potential
     * race in destruction:
     * T1:  enqueue(cleanup_functor) adds the functor to the queue.
     * T2:  The wl_event_loop is destroyed, calling EventLoopDestroyedHandler::on_destroyed()
     * T2:  on_destroyed() drains the workqueue, calling cleanup_functor() and so unregistering
     *      the source.
     * T2: on_destroyed() checks the refcount of work_queue, finding that it's not unique.
     * T2: on_destroyed() unregisters the source, causing a double-free
     * T1: ~WaylandExecutor completes, dropping the ref on the work queue.
     */
    auto const raw_state = state.get();
    auto cleanup_functor =
        [state_holder = std::move(state), source = this->source]()
        {
            wl_event_source_remove(source);
        };

    raw_state->enqueue_termination(std::move(cleanup_functor));

    if (auto err = eventfd_write(notify_fd, 1))
    {
        mir::log_critical(
            "Failed to create event notification for ~WaylandExecutor: %s (%i)",
            strerror(err),
            err);
    }
}

void mf::WaylandExecutor::spawn (std::function<void()>&& work)
{
    state->enqueue(std::move(work));

    if (auto err = eventfd_write(notify_fd, 1))
    {
        BOOST_THROW_EXCEPTION(
            (std::system_error{err, std::system_category(), "eventfd_write failed to notify event loop"}));
    }
}

