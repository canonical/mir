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

#include <thread>
#include <semaphore>
#include <array>
#include <memory>
#include <atomic>
#include <list>

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
          work_available{0},
          thread{&work_loop, this}
    {
    }

    ~Worker() noexcept
    {
        if (thread.joinable())
            stop();
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
        shutdown = true;
        if (work_available.try_acquire())
        {
            // We had some work that we hadn't quite got to, but let's drop that on the floor.
            work = [](){};
        }
        // work_available is now guaranteed not-signalled, as try_acquire has consumed any pending signal,
        // so it's OK to unconditionally raise the signal and so release the workloop thread.
        // (It would be UB to raise the signal if it were already rasied)
        work_available.release();
        thread.join();
    }
private:
    static void work_loop(Worker* me)
    {
        while (!me->shutdown)
        {
            me->work_available.acquire();
            try
            {
                me->work();
            }
            catch (...)
            {
                (*exception_handler)();
            }
            me->work = [](){};
        }
    }

    std::function<void()> work;
    std::binary_semaphore work_available;
    std::atomic<bool> shutdown{false};
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
        // Any active workers have a reference to this ThreadPool. Avoid use-after-free by explicitly
        // cleaning up the workers first.
        for (auto worker : workers)
        {
            worker->stop();
        }
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
                work();
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
        if (num_workers_free <= min_threadpool_threads)
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

mir::SystemExecutor& mir::system_executor = system_threadpool;

void mir::SystemExecutor::spawn(std::function<void()>&& work)
{
    system_threadpool.spawn(std::move(work));
}

void mir::SystemExecutor::set_unhandled_exception_handler(void (*handler)())
{
    exception_handler = handler;
}
