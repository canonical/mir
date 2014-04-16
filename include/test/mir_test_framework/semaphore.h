/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_SEMAPHORE_H_
#define MIR_SEMAPHORE_H_

#include <mutex>
#include <condition_variable>
#include <chrono>

namespace mir_test_framework
{
// Quite why C++11 doesn't have an existing std::semaphore is a mystery
class Semaphore
{
public:
    Semaphore();

    void raise();
    bool raised();

    void wait(void);
    template<typename rep, typename period>
    bool wait_for(std::chrono::duration<rep, period> delay)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        return cv.wait_for(lock, delay, [this]() { return signalled; });
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool signalled;
};
}

#endif // MIR_SEMAPHORE_H_
