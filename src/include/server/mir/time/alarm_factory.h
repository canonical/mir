/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */


#ifndef MIR_TIME_ALARM_FACTORY_H_
#define MIR_TIME_ALARM_FACTORY_H_

#include "mir/time/alarm.h"

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{
class LockableCallback;

namespace time
{

class Alarm;
class AlarmFactory
{
public:
    virtual ~AlarmFactory() = default;
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
     * A LockableCallback allows the user to preserve lock ordering
     * in situations where Alarm methods need to be called under external lock
     * and the callback implementation needs to run code protected by the same
     * lock. An alarm implementation may have internal locks of its own, which
     * maybe acquired during callback dispatching; to preserve lock ordering
     * LockableCallback::lock is invoked during callback dispatch before
     * any internal locks are acquired.
     *
     * \param callback Function to call when the Alarm signals
     * \return A handle to an Alarm that can later be scheduled
     */
    virtual std::unique_ptr<Alarm> create_alarm(std::unique_ptr<LockableCallback> callback) = 0;

protected:
    AlarmFactory() = default;
    AlarmFactory(AlarmFactory const&) = delete;
    AlarmFactory& operator=(AlarmFactory const&) = delete;
};

}
}

#endif // MIR_TIME_ALARM_FACTORY_H_
