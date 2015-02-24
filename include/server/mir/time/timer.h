/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */


#ifndef MIR_TIME_TIMER_H_
#define MIR_TIME_TIMER_H_

#include "mir/time/alarm.h"

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{
namespace time
{

class Alarm;

class Timer
{
public:
    Timer() = default;
    virtual ~Timer() = default;
    /**
     * \brief Create an Alarm that calls the callback after the specified delay
     *
     * \param delay     Time from now, in milliseconds, that the callback will fire
     * \param callback  Function to call when the Alarm signals
     *
     * \return A handle to an Alarm that will fire after delay ms.
     */
    virtual std::unique_ptr<Alarm> notify_in(std::chrono::milliseconds delay,
                                             std::function<void()> const& callback) = 0;
    /**
     * \brief Create an Alarm that calls the callback at the specified time
     *
     * \param time_point Time point when the alarm should be triggered
     * \param callback  Function to call when the Alarm signals
     *
     * \return A handle to an Alarm that will fire after delay ms.
     */
    virtual std::unique_ptr<Alarm> notify_at(Timestamp time_point,
                                             std::function<void()> const& callback) = 0;
    /**
     * \brief Create an Alarm that will not fire until scheduled
     *
     * \param callback  Function to call when the Alarm signals
     *
     * \return A handle to an Alarm that can later be scheduled
     */
    virtual std::unique_ptr<Alarm> create_alarm(std::function<void()> const& callback) = 0;

    /**
     * \brief Create an Alarm that will not fire until scheduled
     *
     * The lock/unlock functions allow the user to preserve lock ordering
     * in situations where Alarm methods need to be called under external lock
     * and the callback implementation needs to run code protected by the same
     * lock. An alarm implementation may have internal locks of its own, which
     * maybe acquired during callback dispatching; to preserve lock ordering
     * the given lock function will be invoked during callback dispatch before
     * any internal locks are acquired.
     *
     * \param callback Function to call when the Alarm signals
     * \param lock     Function called within callback dispatching context
     *                 before the alarm implementation acquires any internal
     *                 lock
     * \param unlock   Function called within callback dispatching context after
     *                 the alarm implementation releases any internal lock
     *
     * \return A handle to an Alarm that can later be scheduled
     */
    virtual std::unique_ptr<Alarm> create_alarm(std::function<void()> const& callback,
        std::function<void()> const& lock, std::function<void()> const& unlock) = 0;

    Timer(Timer const&) = delete;
    Timer& operator=(Timer const&) = delete;
};

}
}

#endif // MIR_TIME_TIMER_H_
