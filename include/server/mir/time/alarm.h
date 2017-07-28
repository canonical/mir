/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_TIME_ALARM_H_
#define MIR_TIME_ALARM_H_

#include "mir/time/types.h"

namespace mir
{
namespace time
{

/**
 * A one-shot, resettable handle to trigger a callback at a later time
 * \note All members of Alarm are threadsafe
 * \note All members of Alarm are safe to call from the Alarm's callback
 */
class Alarm
{
public:
    enum State
    {
        pending,    /**< Will trigger the callback at some point in the future */
        cancelled,  /**< The callback has been cancelled before being triggered */
        triggered   /**< The callback has been called */
    };


    Alarm() = default;
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
     * \return         True if this reschedule supersedes a previous not-yet-triggered timeout
     *
     * \note This cancels any previous timeout set.
     */
    virtual bool reschedule_in(std::chrono::milliseconds delay) = 0;

    /**
     * Reschedule the alarm
     * \param timeout  Time point when the alarm should be triggered
     * \return         True if this reschedule supersedes a previous not-yet-triggered timeout
     *
     * \note This cancels any previous timeout set.
     */
    virtual bool reschedule_for(Timestamp timeout) = 0;

    Alarm(Alarm const&) = delete;
    Alarm& operator=(Alarm const&) = delete;
};

}
}
#endif
