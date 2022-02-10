/*
 * Copyright © 2022 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/linearising_executor.h"
#include "mir/system_executor.h"

#include <deque>
#include <mutex>
#include <thread>

namespace
{
class LinearisingAdaptor : public mir::Executor
{
public:
    LinearisingAdaptor() noexcept
        : idle{true}
    {
    }

    ~LinearisingAdaptor() noexcept
    {
        std::unique_lock<decltype(mutex)> lock{mutex};
        workqueue.clear();
        while (!idle)
        {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            lock.lock();
        }
    }

    void spawn(std::function<void()>&& work) override
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        workqueue.push_back(std::move(work));
        if (idle)
        {
            idle = false;
            mir::system_executor.spawn([this]() { work_loop(); });
        }
    }

private:
    // Execute items from the queue one at a time, until none are left
    void work_loop()
    {
        std::unique_lock<decltype(mutex)> lock{mutex};
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

    std::mutex mutex;
    std::deque<std::function<void()>> workqueue;
    bool idle;
} adaptor;
}

mir::Executor& mir::linearising_executor = adaptor;
