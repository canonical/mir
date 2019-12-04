/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_EXPLICIT_EXECUTOR_H_
#define MIR_TEST_DOUBLES_EXPLICIT_EXECUTOR_H_

#include "mir/executor.h"

#include <mutex>

namespace mir
{
namespace test
{
namespace doubles
{

class ExplicitExectutor
    : public Executor
{
public:
    void spawn(std::function<void()>&& work) override
    {
        std::lock_guard<std::mutex> lock{mutex};
        work_items.push_back(std::move(work));
    }

    void execute()
    {
        std::unique_lock<std::mutex> lock{mutex};
        auto const items = std::move(work_items);
        work_items.clear();
        lock.unlock();
        for (auto const& work : items)
        {
            work();
        }
    }

private:
    std::vector<std::function<void()>> work_items;
    std::mutex mutex;
};

}
}
}

#endif // MIR_TEST_DOUBLES_EXPLICIT_EXECUTOR_H_
