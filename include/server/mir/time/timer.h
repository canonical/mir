/*
 * Copyright © 2014 Canonical Ltd.
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
                                             std::function<void()> callback) = 0;
    /**
     * \brief Create an Alarm that calls the callback at the specified time
     *
     * \param time_point Time point when the alarm should be triggered
     * \param callback  Function to call when the Alarm signals
     *
     * \return A handle to an Alarm that will fire after delay ms.
     */
    virtual std::unique_ptr<Alarm> notify_at(Timestamp time_point,
                                             std::function<void()> callback) = 0;
    /**
     * \brief Create an Alarm that will not fire until scheduled
     *
     * \param callback  Function to call when the Alarm signals
     *
     * \return A handle to an Alarm that can later be scheduled
     */
    virtual std::unique_ptr<Alarm> create_alarm(std::function<void()> callback) = 0;

    Timer(Timer const&) = delete;
    Timer& operator=(Timer const&) = delete;
};

}
}

#endif // MIR_TIME_TIMER_H_
