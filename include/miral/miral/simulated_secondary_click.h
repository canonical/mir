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

#ifndef MIRAL_SIMULATED_SECONDARY_CLICK_H
#define MIRAL_SIMULATED_SECONDARY_CLICK_H

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
class SimulatedSecondaryClick
{
public:
    /// \note `--enable-simulated-secondary-click` has higher precedence than
    /// [enabled_by_default]
    explicit SimulatedSecondaryClick(bool enabled_by_default);

    void operator()(mir::Server& server);

    /// Enables simulated secondary click. If `on_enable` was supplied with a
    /// callback, then it will be called when enabled from a disabled state, or
    /// on assignment if already enabled. When already enabled, further calls
    /// are idempotent.
    SimulatedSecondaryClick& enable();

    /// Disables simulated secondary click. If `on_disable` was supplied with a
    /// callback, then it will be called when disabled from an enabled state,
    /// or on assignment if already disabled. When already disabled, further
    /// calls are idempotent.
    SimulatedSecondaryClick& disable();

    /// Configures the duration users have to keep the left mouse button down
    /// to dispatch a secondary click.
    SimulatedSecondaryClick& hold_duration(std::chrono::milliseconds hold_duration);

    /// Configures the maximum displacement the mouse pointer can move before
    /// the simulated secondary click is cancelled.
    SimulatedSecondaryClick& displacement_threshold(float displacement);

    /// Configures the callback invoked when simulated secondary click is
    /// transitions from being disabled to being enabled. Will be called on
    /// assignment if simulated secondary click is already enabled. After
    /// assignment, if simulated secondary click is already enabled and
    /// `enaable()` is called again, the supplied callback will not be invoked.
    SimulatedSecondaryClick& on_enabled(std::function<void()>&& on_enabled);

    /// Configures the callback invoked when simulated secondary click is
    /// transitions from being enabled to being disabled. Will be called on
    /// assignment if simulated secondary click is already disabled. After
    /// assignment, if simulated secondary click is already disabled and
    /// `disable()` is called again, the supplied callback will not be invoked.
    SimulatedSecondaryClick& on_disabled(std::function<void()>&& on_disabled);

    /// Configures the callback to invoke when the user clicks _down_ the left
    /// mouse button. Can be used to initialize animations for graphical feedback.
    SimulatedSecondaryClick& on_hold_start(std::function<void()>&& on_hold_start);

    /// Configures the callback to invoke when the user cancels a simulated
    /// secondary click either by letting go of the left button before the hold
    /// duration is up or moving the cursor. Can be used to indicate to the
    /// user that the action was cancelled.
    SimulatedSecondaryClick& on_hold_cancel(std::function<void()>&& on_hold_cancel);

    /// Configures the callback to invoke after the user successfully
    /// dispatches a simulated secondary click. Can be used to indicate to the
    /// user that their simulated click was successful.
    SimulatedSecondaryClick& on_secondary_click(std::function<void()>&& on_secondary_click);

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_SIMULATED_SECONDARY_CLICK_H
