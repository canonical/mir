/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/executor.h>

#include <deque>
#include <mutex>
#include <condition_variable>

namespace
{
class LinearisingAdaptor : public mir::NonBlockingExecutor
{
public:
    LinearisingAdaptor() noexcept
        : idle{true}
    {
    }

    ~LinearisingAdaptor() noexcept
    {
        std::unique_lock lock{mutex};
        workqueue.clear();
        while (!idle)
        {
            idle_changed.wait(lock);
        }
    }

    void spawn(std::function<void()>&& work) override
    {
        std::lock_guard lock{mutex};
        workqueue.push_back(std::move(work));
        if (idle)
        {
            idle = false;
            mir::thread_pool_executor.spawn([this]() { work_loop(); });
        }
    }

private:
    // Execute items from the queue one at a time, until none are left
    void work_loop()
    {
        {
            std::unique_lock lock{mutex};
            while (!workqueue.empty())
            {
                {
                    auto work = std::move(workqueue.front());
                    workqueue.pop_front();
                    lock.unlock();
                    work();
                }
                lock.lock();
            }
            idle = true;
        }
        idle_changed.notify_one();
    }

    std::mutex mutex;
    std::condition_variable idle_changed;
    std::deque<std::function<void()>> workqueue;
    bool idle;
} adaptor;
}

mir::NonBlockingExecutor& mir::linearising_executor = adaptor;
