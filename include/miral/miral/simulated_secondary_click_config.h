/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_SIMULATED_SECONDARY_CLICK_CONFIG_H
#define MIRAL_SIMULATED_SECONDARY_CLICK_CONFIG_H

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{
class Server;
}

namespace miral
{
/// Enables configuring simulated secondary click at runtime.
/// \remark Since MirAL 5.3
class SimulatedSecondaryClickConfig
{
public:
    /// \note `--enable-simulated-secondary-click` has higher precedence than
    /// [enabled_by_default]
    SimulatedSecondaryClickConfig(bool enabled_by_default);

    void operator()(mir::Server& server);

    /// Enables simulated secondary click
    SimulatedSecondaryClickConfig& enable();

    /// Disables simulated secondary click
    SimulatedSecondaryClickConfig& disable();

    /// Configures the duration users have to keep the left mouse button down
    /// to dispatch a secondary click
    SimulatedSecondaryClickConfig& hold_duration(std::chrono::milliseconds hold_duration);

    /// Configures the maximum displacement the mouse pointer can move before
    /// the simulated secondary click is cancelled.
    SimulatedSecondaryClickConfig& displacement_threshold(float displacement);

    /// Configures the callbacks invoked when simulated secondary click is
    /// enabled or disabled
    SimulatedSecondaryClickConfig& enabled(std::function<void()>&& on_enabled);
    SimulatedSecondaryClickConfig& disabled(std::function<void()>&& on_disabled);

    /// Configures the callback to invoke when the user clicks _down_ the left
    /// mouse button
    SimulatedSecondaryClickConfig& hold_start(std::function<void()>&& on_hold_start);

    /// Configures the callback to invoke when the user cancels a simulated
    /// secondary click either by letting go of the left button before the
    /// hold duration is up or moving the cursor
    SimulatedSecondaryClickConfig& hold_cancel(std::function<void()>&& on_hold_cancel);

    /// Configures the callback to invoke when the user successfully
    /// dispatches a simulated secondary click
    SimulatedSecondaryClickConfig& secondary_click(std::function<void()>&& on_secondary_click);

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_SIMULATED_SECONDARY_CLICK_CONFIG_H
