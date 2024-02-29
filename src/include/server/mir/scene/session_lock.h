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

#ifndef MIR_FRONTEND_SESSION_LOCK_H
#define MIR_FRONTEND_SESSION_LOCK_H

#include "mir/observer_registrar.h"
#include <memory>

namespace mir
{
class Executor;

namespace scene
{

/// Used to observe session locks and unlocks
class SessionLockObserver
{
public:
    SessionLockObserver() = default;
    virtual ~SessionLockObserver() = default;
    SessionLockObserver(SessionLockObserver const&) = delete;
    SessionLockObserver& operator=(SessionLockObserver const&) = delete;
    virtual void on_lock() = 0;
    virtual void on_unlock() = 0;
};

class SessionLock : public ObserverRegistrar<SessionLockObserver>
{
public:
    SessionLock() = default;
    virtual ~SessionLock() = default;
    SessionLock (SessionLock const&) = delete;
    SessionLock& operator= (SessionLock const&) = delete;

    virtual void lock() = 0;
    virtual void unlock() = 0;
};

}
}

#endif //MIR_FRONTEND_SESSION_LOCK_H
