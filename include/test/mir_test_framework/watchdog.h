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

#ifndef MIR_WATCHDOG_H_
#define MIR_WATCHDOG_H_

#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace mir_test_framework
{
class WatchDog
{
public:
    WatchDog(std::function<void(void)> killer);
    ~WatchDog() noexcept;
    void run(std::function<void(WatchDog&)> watchee);
    void notify_done();
    void stop();

    template<typename rep, typename period>
    bool wait_for(std::chrono::duration<rep, period> delay);
private:
    std::thread runner;
    std::mutex m;
    std::condition_variable notifier;
    bool done;
    std::function<void(void)> killer;
};

template<typename rep, typename period>
bool WatchDog::wait_for(std::chrono::duration<rep, period> delay)
{
    std::unique_lock<decltype(m)> lock(m);
    return notifier.wait_for(lock, delay, [this](){return done;});
}
}

#endif // MIR_WATCHDOG_H_
