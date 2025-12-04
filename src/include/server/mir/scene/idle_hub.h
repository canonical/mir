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

#ifndef MIR_SCENE_IDLE_HUB_H_
#define MIR_SCENE_IDLE_HUB_H_

#include <mir/time/types.h>

#include <memory>
#include <optional>
#include <string>

namespace mir
{
class NonBlockingExecutor;
namespace scene
{
/// Gets notifications about the idle state when registered and subsequent state changes until unregistered
class IdleStateObserver
{
public:
    IdleStateObserver() = default;
    virtual ~IdleStateObserver() = default;

    virtual void idle() = 0;
    virtual void active() = 0;

private:
    IdleStateObserver(IdleStateObserver const&) = delete;
    IdleStateObserver& operator=(IdleStateObserver const&) = delete;
};

/// Interface that exposes notifications about when Mir is idle, and allows for waking Mir up
class IdleHub
{
public:
    IdleHub() = default;
    virtual ~IdleHub() = default;

    /// Lock that is held by the surface which is inhibiting idle
    struct WakeLock;

    /// Wakes Mir if it's not already awake, and starts the timer from now
    virtual void poke() = 0;

    /// Timeout is the amount of time Mir will stay idle before triggering the observer
    virtual void register_interest(
        std::weak_ptr<IdleStateObserver> const& observer,
        time::Duration timeout) = 0;

    /// Timeout is the amount of time Mir will stay idle before triggering the observer
    virtual void register_interest(
        std::weak_ptr<IdleStateObserver> const& observer,
        NonBlockingExecutor& executor,
        time::Duration timeout) = 0;

    virtual void unregister_interest(IdleStateObserver const& observer) = 0;

    virtual auto inhibit_idle() -> std::shared_ptr<IdleHub::WakeLock> = 0;

private:
    IdleHub(IdleHub const&) = delete;
    IdleHub& operator=(IdleHub const&) = delete;
};
}
}

#endif // MIR_SCENE_IDLE_HUB_H_
