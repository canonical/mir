/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_SIGNAL_H_
#define MIR_TEST_SIGNAL_H_

#include <condition_variable>
#include <chrono>
#include <mutex>

namespace mir
{
namespace test
{
/**
 * @brief A threadsafe, waitable signal
 */
class Signal
{
public:
    void raise();
    bool raised();

    void wait();
    template<typename rep, typename period>
    bool wait_for(std::chrono::duration<rep, period> delay)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        return cv.wait_for(lock, delay, [this]() { return signalled; });
    }
    template<class Clock, class Duration>
    bool wait_until(std::chrono::time_point<Clock, Duration> const& time)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        return cv.wait_until(lock, time, [this]() { return signalled; });
    }

    void reset();

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool signalled{false};
};
}
}

#endif // MIR_TEST_SIGNAL_H_
