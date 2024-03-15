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

#ifndef MIR_TEST_DOUBLES_EXPLICIT_EXECUTOR_H_
#define MIR_TEST_DOUBLES_EXPLICIT_EXECUTOR_H_

#include "mir/executor.h"
#include "gtest/gtest.h"

#include <mutex>
#include <thread>

namespace mir
{
namespace test
{
namespace doubles
{

class ExplicitExecutor
    : public NonBlockingExecutor
{
public:
    ~ExplicitExecutor()
    {
        if (!work_items.empty())
        {
            ADD_FAILURE() <<
                "ExplicitExecutor destroyed with " <<
                work_items.size() <<
                " work items left in the queue";
        }
    }

    void spawn(std::function<void()>&& work) override
    {
        std::lock_guard lock{mutex};
        work_items.push_back(std::move(work));
    }

    void execute()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        decltype(work_items) drained_items;
        do
        {
            drained_items.clear();
            {
                std::lock_guard lock{mutex};
                swap(drained_items, work_items);
            }
            for (auto const& work : drained_items)
            {
                work();
            }
        } while (!drained_items.empty());
    }

private:
    std::vector<std::function<void()>> work_items;
    std::mutex mutex;
};

}
}
}

#endif // MIR_TEST_DOUBLES_EXPLICIT_EXECUTOR_H_
