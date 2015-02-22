/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/thread/basic_thread_pool.h"
#include "mir/terminate_with_current_exception.h"

#include <deque>
#include <algorithm>
#include <condition_variable>

namespace mt = mir::thread;

namespace
{

class Task
{
public:
    Task(std::function<void()> const& task) : task{task} {}

    void execute()
    {
        try
        {
            task();
        }
        catch (...)
        {
            task_exception = std::current_exception();
        }
    }

    void notify_done()
    {
        if (task_exception)
            promise.set_exception(task_exception);
        else
            promise.set_value();
    }

    std::future<void> get_future()
    {
        return promise.get_future();
    }

private:
    std::function<void()> const task;
    std::promise<void> promise;
    std::exception_ptr task_exception;
};

class Worker
{
public:
    Worker() : exiting{false}
    {
    }

    ~Worker()
    {
        exit();
    }

    void operator()() noexcept
    try
    {
       std::unique_lock<std::mutex> lock{state_mutex};
       while (!exiting)
       {
           task_available_cv.wait(lock, [&]{ return exiting || !tasks.empty(); });

           if (!exiting)
           {
               auto task = std::move(tasks.front());
               lock.unlock();
               task.execute();
               lock.lock();
               tasks.pop_front();
               task.notify_done();
           }
       }
    }
    catch(...)
    {
        mir::terminate_with_current_exception();
    }

    void queue_task(Task task)
    {
        std::lock_guard<std::mutex> lock{state_mutex};
        tasks.push_back(std::move(task));
        task_available_cv.notify_one();
    }

    void exit()
    {
        std::lock_guard<std::mutex> lock{state_mutex};
        exiting = true;
        task_available_cv.notify_one();
    }

    bool is_idle() const
    {
        std::lock_guard<std::mutex> lock{state_mutex};
        return tasks.empty();
    }

private:
    std::deque<Task> tasks;
    bool exiting;
    std::mutex mutable state_mutex;
    std::condition_variable task_available_cv;
};

}

namespace mir
{
namespace thread
{
class WorkerThread
{
public:
    WorkerThread(mt::BasicThreadPool::TaskId an_id)
        : thread{std::ref(worker)},
          id_{an_id}
    {}

    ~WorkerThread()
    {
        worker.exit();
        if (thread.joinable())
            thread.join();
    }

    void queue_task(Task task, mt::BasicThreadPool::TaskId the_id)
    {
        worker.queue_task(std::move(task));
        id_ = the_id;
    }

    bool is_idle() const
    {
        return worker.is_idle();
    }

    mt::BasicThreadPool::TaskId current_id() const
    {
        return id_;
    }

private:
    ::Worker worker;
    std::thread thread;
    mt::BasicThreadPool::TaskId id_;
};
}
}

mt::BasicThreadPool::BasicThreadPool(int min_threads)
    : min_threads{min_threads}
{
}

mt::BasicThreadPool::~BasicThreadPool() = default;

std::future<void> mt::BasicThreadPool::run(std::function<void()> const& task)
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    TaskId const generic_id = nullptr;
    WorkerThread* no_preferred_thread = nullptr;
    return run(no_preferred_thread, task, generic_id);
}

std::future<void> mt::BasicThreadPool::run(std::function<void()> const& task, TaskId id)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    return run(find_thread_by(id), task, id);
}

std::future<void> mt::BasicThreadPool::run(WorkerThread* worker_thread,
                                           std::function<void()> const& task, TaskId id)
{
    // No pre-selected thread to execute task, find an idle thread
    if (worker_thread == nullptr)
        worker_thread = find_idle_thread();

    if (worker_thread == nullptr)
    {
        // No idle threads available so create a new one
        auto new_worker_thread = std::make_unique<WorkerThread>(id);
        threads.push_back(std::move(new_worker_thread));
        worker_thread = threads.back().get();
    }

    Task a_task{task};
    auto future = a_task.get_future();
    worker_thread->queue_task(std::move(a_task), id);
    return future;
}


void mt::BasicThreadPool::shrink()
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    int max_threads_to_remove = threads.size() - min_threads;
    auto it = std::remove_if(threads.begin(), threads.end(),
        [&max_threads_to_remove](std::unique_ptr<WorkerThread> const& worker_thread)
        {
            bool remove = worker_thread->is_idle() && max_threads_to_remove > 0;
            if (remove)
                max_threads_to_remove--;
            return remove;
        }
    );
    threads.erase(it, threads.end());
}

mt::WorkerThread* mt::BasicThreadPool::find_thread_by(TaskId id)
{
    auto it = std::find_if(threads.begin(), threads.end(),
        [id](std::unique_ptr<WorkerThread> const& worker_thread)
        {
            return worker_thread->current_id() == id;
        }
    );

    return it == threads.end() ? nullptr : it->get();
}

mt::WorkerThread *mt::BasicThreadPool::find_idle_thread()
{
    auto it = std::find_if(threads.begin(), threads.end(),
        [](std::unique_ptr<WorkerThread> const& worker_thread)
        {
            return worker_thread->is_idle();
        }
    );

    return it == threads.end() ? nullptr : it->get();
}
