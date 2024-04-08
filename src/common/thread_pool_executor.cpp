/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/executor.h"

#include "mir/thread_name.h"

#include <thread>
#include <memory>
#include <atomic>
#include <list>
#include <future>
#include <semaphore>

namespace
{
constexpr int const min_threadpool_threads = 4;

/* We use an atomic void(*)() rather than a std::function to avoid needing to take a mutex
 * in exception context, as taking a mutex can itself throw an exception!
 */
std::atomic<void(*)()> exception_handler{[] { std::rethrow_exception(std::current_exception()); }};

class Worker
{
public:
    Worker()
        : work{[](){}},
          work_available{0}
    {
        std::promise<std::atomic<bool>*> shutdown_channel;
        auto resolved_shutdown_location = shutdown_channel.get_future();

        thread = std::thread{
            &work_loop,
            this,
            std::move(shutdown_channel)};

        shutdown = resolved_shutdown_location.get();
    }

    ~Worker() noexcept
    {
        if (thread.joinable())
        {
            /* Safety: stop() is the only other callsite of signal_work_thread_to_halt()
             * If stop() has been called then it has already called thread.join(), so
             * thread.joinable() will be false and we can't get here.
             */
            signal_work_thread_to_halt();
            if (thread.get_id() == std::this_thread::get_id())
            {
                /* We're being destroyed from our own work thread.
                 * This can only happen when destroying the work functor at me->work = [](){}
                 * in work_loop results in destroying the last shared pointer to *this.
                 *
                 * At this point, work_loop will exit without touching anything
                 */
                thread.detach();
            }
            else
            {
                thread.join();
            }
        }
    }

    void set_work(std::function<void()>&& to_do)
    {
        // Precondition: this Worker is idle (work_available is unraised, work = [](){})
        work = std::move(to_do);
        work_available.release();
    }

    /**
     * Halt the worker, dropping any work scheduled but not yet executing, and wait for completion.
     */
    void stop()
    {
        signal_work_thread_to_halt();
        thread.join();
    }
private:
    void signal_work_thread_to_halt()
    {
        /* Precondition: *shutdown == false
         * If *shutdown has become true, then we are unsynchronised with the termination of
         * the worker thread, and so we can not guarantee that shutdown points to valid memory.
         *
         * This implies that signal_work_thread_to_halt() must be called exactly once.
         */
        *shutdown = true;
        /* We need to release the workloop thread by raising the signal.
         * However, it's UB to raise the semaphore if it's already raised.
         * So, first, consume any pending signal…
         */
        work_available.try_acquire();
        /* work_available is now guaranteed not-signalled, so it's OK to
         * unconditionally raise the signal and so release the workloop thread.
         */
        work_available.release();
    }

    static void work_loop(Worker* me, std::promise<std::atomic<bool>*>&& shutdown_channel)
    {
        mir::set_thread_name("Mir/Workqueue");

        std::atomic<bool> shutdown{false};
        shutdown_channel.set_value(&shutdown);

        while (!shutdown)
        {
            me->work_available.acquire();
            auto work = std::move(me->work);
            me->work = [](){};
            work();
        }
    }

    std::function<void()> work;
    /**
     * Pointer to an atomic<bool> on the thread's stack (in work_loop).
     *
     * Being on the thread's stack means that the thread is allowed to outlive the Worker
     * (as long as the thread does not access anything from the Worker), but also means that
     * after setting *shutdown to true it is no longer safe to access.
     */
    std::atomic<bool>* shutdown;
    std::binary_semaphore work_available;
    std::thread thread;
};

/**
 * A self-managing ThreadPool
 *
 * Theory of operation:
 * The ThreadPool executes each item of work on an independent thread; each call to spawn() is
 * guaranteed to be on a different thread to the caller.
 *
 * To reduce the overhead of spawning threads, the ThreadPool attempts to maintain
 * min_threadpool_threads of free worker threads.
 *
 * The ThreadPool maintains two linked-lists of Worker threads; a list of all Workers, and a list of
 * (Worker, position-in-all-workers-list) containing idle Worker threads.
 *
 * When a work item is received through spawn() the ThreadPool first checks to see if there's a
 * free Worker thread in the idle list; if so, it takes it off the free list and dispatches the work.
 * If there are no free Workers, it creates a new Worker, adds it to the all-workers list, and dispatches
 * the work to the new Worker.
 *
 * When a Worker finishes an item of work, it calls recycle(). If there are fewer than
 * min_threadpool_threads in the free list, the Worker is added to the free list. Otherwise, the
 * Worker is removed from the all-workers list and is destroyed.
 */
class ThreadPool : public mir::NonBlockingExecutor
{
public:
    ThreadPool() noexcept
    {
    }

    ~ThreadPool() noexcept
    {
        wait_for_idle();
    }

    void quiesce()
    {
        wait_for_idle();
        std::lock_guard lock{workers_mutex};
        free_workers.clear();
        workers.clear();
        num_workers_free = 0;
    }

    void spawn(std::function<void()>&& work)
    {
        WorkerHandle worker;
        {
            std::lock_guard lock{workers_mutex};
            if (num_workers_free > 0)
            {
                worker = free_workers.front();
                free_workers.pop_front();
                num_workers_free--;
            }
            else
            {
                worker.worker = std::make_shared<Worker>();
                worker.pos = workers.insert(workers.cend(), worker.worker);
            }
        }
        worker.worker->set_work(
            [this, worker, work = std::move(work)]() mutable
            {
                try
                {
                    work();
                }
                catch (...)
                {
                    (*exception_handler)();
                }
                recycle(std::move(worker));
            });
    }
private:
    struct WorkerHandle
    {
        std::list<std::shared_ptr<Worker>>::const_iterator pos;
        std::shared_ptr<Worker> worker;
    };

    void wait_for_idle()
    {
        std::unique_lock lock{workers_mutex};
        // We need to wait for any active workers to finish.
        bool idle = workers.size() == free_workers.size();
        while (!idle)
        {
            workers_changed.wait(lock);

            /* When a worker finishes it either adds itself to the free_workers list
             * or removes itself from the workers list.
             *
             * This means that we're idle once all the workers are free_workers
             * - that is, the free_workers list is the same size as workers.
             */
            idle = workers.size() == free_workers.size();
        }
    }

    void recycle(WorkerHandle&& worker)
    {
        {
            std::lock_guard lock{workers_mutex};
            if (num_workers_free < min_threadpool_threads)
            {
                // If we're below our free-thread minimum, recycle this thread back into the pool…
                free_workers.emplace_front(std::move(worker));
                num_workers_free++;
            }
            else
            {
                // …otherwise, let it die.
                workers.erase(worker.pos);
            }
        }
        workers_changed.notify_all();
    }

    std::mutex workers_mutex;
    std::condition_variable workers_changed;
    int num_workers_free{0};
    std::list<WorkerHandle> free_workers;
    std::list<std::shared_ptr<Worker>> workers;
};

ThreadPool thread_pool;

}

mir::NonBlockingExecutor& mir::thread_pool_executor = thread_pool;

void mir::ThreadPoolExecutor::spawn(std::function<void()>&& work)
{
    thread_pool.spawn(std::move(work));
}

void mir::ThreadPoolExecutor::set_unhandled_exception_handler(void (*handler)())
{
    exception_handler = handler;
}

void mir::ThreadPoolExecutor::quiesce()
{
    thread_pool.quiesce();
}
