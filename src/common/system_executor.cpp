/*
 * Copyright © 2022 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/system_executor.h"

#include "mir/thread_name.h"

#include <thread>
#include <semaphore>
#include <array>
#include <memory>
#include <atomic>
#include <list>
#include <future>

namespace
{
constexpr int const min_threadpool_threads = 4;

void just_rethrow_exception()
{
    std::rethrow_exception(std::current_exception());
}

/* We use an atomic void(*)() rather than a std::function to avoid needing to take a mutex
 * in exception context, as taking a mutex can itself throw an exception!
 */
std::atomic<void(*)()> exception_handler = &just_rethrow_exception;

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
    std::atomic<bool>* shutdown;
    std::binary_semaphore work_available;
    std::thread thread;
};

class ThreadPool : public mir::SystemExecutor
{
public:
    ThreadPool() noexcept
    {
    }

    ~ThreadPool() noexcept
    {
        // We need to wait for any active workers to finish.
        bool idle;
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});

            std::lock_guard<decltype(workers_mutex)> lock{workers_mutex};
            /* When a worker finishes it either adds itself to the free_workers list
             * or removes itself from the workers list.
             *
             * This means that we're idle once all the workers are free_workers
             * - that is, the free_workers list is the same size as workers.
             */
            idle = workers.size() == free_workers.size();
        } while(!idle);
    }

    void spawn(std::function<void()>&& work)
    {
        WorkerHandle worker;
        {
            std::lock_guard<decltype(workers_mutex)> lock{workers_mutex};
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

    void recycle(WorkerHandle&& worker)
    {
        std::lock_guard<decltype(workers_mutex)> lock{workers_mutex};
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

    std::mutex workers_mutex;
    int num_workers_free{0};
    std::list<WorkerHandle> free_workers;
    std::list<std::shared_ptr<Worker>> workers;
};

ThreadPool system_threadpool;

}

mir::Executor& mir::system_executor = system_threadpool;

void mir::SystemExecutor::spawn(std::function<void()>&& work)
{
    system_threadpool.spawn(std::move(work));
}

void mir::SystemExecutor::set_unhandled_exception_handler(void (*handler)())
{
    exception_handler = handler;
}
