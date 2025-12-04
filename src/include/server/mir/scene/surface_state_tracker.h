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

#ifndef MIR_SCENE_SURFACE_STATE_TRACKER_
#define MIR_SCENE_SURFACE_STATE_TRACKER_

#include <mir_toolkit/common.h>
#include <initializer_list>

namespace mir
{
namespace scene
{
/// Represents a surface's current state, as well as the states it will fall back to as states are removed. For example,
/// a minimized surface may also be fullscreen (which will become relevant when it's unminimized) but not maximized
/// (which will become relevant when it's unfullscreened). This class keeps track of all the active and inactive states.
/// States have this order of precedence (earlier ones will be the active state if present):
/// - hidden, minimized, fullscreen, attached, maximized/vertmaximized/horizmaximized, restored
struct SurfaceStateTracker
{
public:
    explicit SurfaceStateTracker(MirWindowState initial);

    /// This class can be freely copied
    SurfaceStateTracker(const SurfaceStateTracker&) = default;
    /// Assignment is not threadsafe, use external locking if required
    SurfaceStateTracker& operator=(const SurfaceStateTracker&) = default;

    /// Returns the currently active state
    auto active_state() const -> MirWindowState;

    /// Returns a copy of this tracker with states added or removed as needed to make the given state active
    [[nodiscard]]
    auto with_active_state(MirWindowState state) const -> SurfaceStateTracker;

    /// Returns if the given state is present
    auto has(MirWindowState state) const -> bool;

    /// Returns if any of the given states are present
    auto has_any(std::initializer_list<MirWindowState> const& states) const -> bool;

    /// Returns a copy of this tracker with the given state added (which may or may not effect the active state).
    /// Sending mir_window_state_restored is NOT allowed, as restored is the baseline state all surfaces always have.
    [[nodiscard]]
    auto with(MirWindowState state) const -> SurfaceStateTracker;

    /// Returns a copy of this tracker with the given state removed (which may or may not effect the active state).
    /// Sending mir_window_state_restored is NOT allowed, as restored is the baseline state all surfaces always have.
    [[nodiscard]]
    auto without(MirWindowState state) const -> SurfaceStateTracker;

private:
    SurfaceStateTracker()
        : hidden{false},
          minimized{false},
          fullscreen{false},
          attached{false},
          horiz_maximized{false},
          vert_maximized{false}
    {
    }

    /// Creates a tracker with the given active state and the minimal possible change from the base tracker
    SurfaceStateTracker(SurfaceStateTracker base, MirWindowState active);

    /// Copies the base tracker with the given state present or not present
    SurfaceStateTracker(SurfaceStateTracker base, MirWindowState state, bool present);

    friend auto operator==(SurfaceStateTracker const& lhs, SurfaceStateTracker const& rhs) -> bool;

    /// Functionally const except for asignment and construction
    /// @{
    bool hidden : 1;
    bool minimized : 1;
    bool fullscreen : 1;
    bool attached : 1;
    bool horiz_maximized : 1;
    bool vert_maximized : 1;
    /// @}
};

/// Returns if all states are equal
auto operator==(SurfaceStateTracker const& lhs, SurfaceStateTracker const& rhs) -> bool;
inline auto operator!=(SurfaceStateTracker const& lhs, SurfaceStateTracker const& rhs) -> bool { return !(lhs == rhs); }
}
}

#endif // MIR_SCENE_SURFACE_STATE_TRACKER_
