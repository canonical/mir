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

#ifndef MIR_THREAD_BASIC_THREAD_POOL_H_
#define MIR_THREAD_BASIC_THREAD_POOL_H_

#include <functional>
#include <future>
#include <vector>
#include <memory>
#include <mutex>

namespace mir
{
namespace thread
{

class WorkerThread;
class BasicThreadPool
{
public:
    BasicThreadPool(int min_threads);
    ~BasicThreadPool();

    std::future<void> run(std::function<void()> const& task);

    typedef void const* TaskId;
    std::future<void> run(std::function<void()> const& task, TaskId id);

    void shrink();

private:
    BasicThreadPool(BasicThreadPool const&) = delete;
    BasicThreadPool& operator=(BasicThreadPool const&) = delete;

    std::future<void> run(WorkerThread* t, std::function<void()> const& task, TaskId id);
    WorkerThread *find_thread_by(TaskId id);
    WorkerThread *find_idle_thread();

    std::mutex mutex;
    int const min_threads;
    std::vector<std::unique_ptr<WorkerThread>> threads;
};

}
}

#endif
