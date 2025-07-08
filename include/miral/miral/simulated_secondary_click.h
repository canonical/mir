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
/// 
/// Simulated secondary click is an accessibility method that allows users to
/// simulate secondary clicks by holding the left mouse button. The pointer is
/// allowed to move slightly during the hold to accomodate users with
/// disabilities.
///
/// \remark Since MirAL 5.3
class SimulatedSecondaryClick
{
public:
    [[deprecated(
        "SimulatedSecondaryClick(bool) is deprecated, please use SimulatedSecondaryClick::enabled or "
        "SimulatedSecondaryClick::disabled")]]
    explicit SimulatedSecondaryClick(bool enabled_by_default);

    /// Creates a `SimulatedSecondaryClick` instance that's enabled by default.
    /// \remark Since MirAL 5.4
    auto static enabled() -> SimulatedSecondaryClick;

    /// Creates a `SimulatedSecondaryClick` instance that's disabled by default.
    /// \remark Since MirAL 5.4
    auto static disabled() -> SimulatedSecondaryClick;

    void operator()(mir::Server& server);

    /// Enables simulated secondary click.
    /// When already enabled, further calls have no effect.
    SimulatedSecondaryClick& enable();

    /// Disables simulated secondary click.
    /// When already disabled, further calls have no effect.
    SimulatedSecondaryClick& disable();

    /// Configures the duration users have to keep the left mouse button down
    /// to dispatch a secondary click.
    SimulatedSecondaryClick& hold_duration(std::chrono::milliseconds hold_duration);

    /// Configures the maximum displacement the mouse pointer can move before
    /// the simulated secondary click is cancelled.
    SimulatedSecondaryClick& displacement_threshold(float displacement);

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
    SimulatedSecondaryClick(std::shared_ptr<Self> self);
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_SIMULATED_SECONDARY_CLICK_H
