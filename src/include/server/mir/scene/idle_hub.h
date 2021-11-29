/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SCENE_IDLE_HUB_H_
#define MIR_SCENE_IDLE_HUB_H_

#include "mir/observer_registrar.h"

#include <memory>
#include <optional>
#include <string>

namespace mir
{
namespace scene
{
enum class IdleState
{
    awake,      ///< Outputs should be on
    dim,        ///< Outputs should be on, but dimmed (may not reduce power usage, but warns the user the screen is
                ///< about to blank)
    standby,    ///< Outputs should be in mir_power_mode_standby
    off,        ///< Outputs should be in mir_power_mode_off
};

/// Gets notifications about changes in the idle state
class IdleStateObserver
{
public:
    IdleStateObserver() = default;
    virtual ~IdleStateObserver() = default;

    virtual void idle_state_changed(IdleState state) = 0;

private:
    IdleStateObserver(IdleStateObserver const&) = delete;
    IdleStateObserver& operator=(IdleStateObserver const&) = delete;
};

/// Interface that exposes notifications about when Mir is idle, and allows for waking Mir up
class IdleHub : public ObserverRegistrar<IdleStateObserver>
{
public:
    virtual ~IdleHub() = default;

    /// Current state
    virtual auto state() -> IdleState = 0;

    /// Wakes Mir if it's not already awake, and starts the timer from now
    virtual void poke() = 0;
};
}
}

#endif // MIR_SCENE_IDLE_HUB_H_
