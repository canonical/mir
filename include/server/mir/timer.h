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


#ifndef MIR_TIMER_H_
#define MIR_TIMER_H_

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{

/**
 * A one-shot, resettable handle to trigger a callback at a later time
 * \note All members of Alarm are threadsafe
 */
class Alarm
{
public:
    enum State
    {
        Pending,    /**< Will trigger the callback at some point in the future */
        Cancelled,  /**< The callback has been cancelled before being triggered */
        Triggered   /**< The callback has been called */
    };

    /**
     * \note Destruction of the Alarm guarantees that the callback will not subsequently be called
     */
    virtual ~Alarm() = default;

    /**
     * Cancels a pending alarm
     *
     * \note Has no effect if the Alarm is in the Triggered state.
     * \note cancel() is idempotent
     *
     * \return True iff the state of the Alarm is now Cancelled
     */
    virtual bool cancel() = 0;
    virtual State state() const = 0;

    /**
     * Reschedule the alarm
     * \param delay    Delay, in milliseconds, before the Alarm will be triggered
     * \return         True iff this reschedule supercedes a previous not-yet-triggered timeout
     *
     * \note This cancels any previous timeout set.
     */
    virtual bool reschedule_in(std::chrono::milliseconds delay) = 0;
};

class Timer
{
public:
    virtual ~Timer() = default;
    /**
     * \brief Create an Alarm that calls the callback at the specified time
     *
     * \param delay     Time from now, in milliseconds, that the callback will fire
     * \param callback  Function to call when the Alarm signals
     *
     * \return A handle to an Alarm that will fire after delay ms.
     */
    virtual std::unique_ptr<Alarm> notify_in(std::chrono::milliseconds delay,
                                             std::function<void()> callback) = 0;
};

}

#endif // MIR_TIMER_H_
