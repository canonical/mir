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

#ifndef MIR_FRONTEND_SESSION_LOCKER_H
#define MIR_FRONTEND_SESSION_LOCKER_H

#include "mir/observer_multiplexer.h"

namespace mir
{
namespace frontend
{

/// Used to observe session locks and unlocks
class SessionLockObserver
{
public:
    SessionLockObserver() = default;
    virtual ~SessionLockObserver() = default;
    virtual void on_lock() = 0;
    virtual void on_unlock() = 0;
};

/// Responsible for triggering session locks and notifying observers
class SessionLocker : public ObserverMultiplexer<SessionLockObserver>
{
protected:
    explicit SessionLocker(Executor& default_executor)
        : ObserverMultiplexer{default_executor}
    {
    }
};

}
}

#endif //MIR_FRONTEND_SESSION_LOCKER_H
