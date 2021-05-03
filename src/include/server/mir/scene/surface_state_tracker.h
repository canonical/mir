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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SCENE_SURFACE_STATE_TRACKER_
#define MIR_SCENE_SURFACE_STATE_TRACKER_

#include "mir_toolkit/common.h"

namespace mir
{
namespace scene
{
/// Represents a surface's current state, as well as the states it will fall back to as states are removed. For example
/// A minimized surface may also be fullscreen (which will become relevant when it's unminimized) but not maximized
/// (which will become relevant when it's unfullscreened). This class keeps track of all the active and inactive states.
/// States have this order of precedence (earlier ones will be the active state if present):
/// - hidden, minimized, fullscreen, attached, maximized/vertmaximized/horizmaximized, restored
/// This struct can NOT be safely accessed by multiple threads at once.
struct SurfaceStateTracker
{
public:
    explicit SurfaceStateTracker(MirWindowState initial)
        : hidden{false},
          minimized{false},
          fullscreen{false},
          attached{false},
          horiz_maximized{false},
          vert_maximized{false}
    {
        set_active_state(initial);
    }
    /// This class can be freely copied and assigned
    SurfaceStateTracker(const SurfaceStateTracker&) = default;
    SurfaceStateTracker& operator=(const SurfaceStateTracker&) = default;

    /// Returns the currently active state
    auto active_state() const -> MirWindowState;

    /// Adds or removes states as needed to make the given state active
    void set_active_state(MirWindowState state);

    /// Returns if the given state is present
    auto has(MirWindowState state) const -> bool;

    /// Adds the given state (which may or may not effect the active state) and returns this object. Sending
    /// mir_window_state_restored is NOT allowed, as restored is the baseline state all surfaces always have.
    auto with(MirWindowState state) -> SurfaceStateTracker&;

    /// Removes the given state (which may or may not effect the active state) and returns this object. Sending
    /// mir_window_state_restored is NOT allowed, as restored is the baseline state all surfaces always have.
    auto without(MirWindowState state) -> SurfaceStateTracker&;

private:
    bool hidden : 1;
    bool minimized : 1;
    bool fullscreen : 1;
    bool attached : 1;
    bool horiz_maximized : 1;
    bool vert_maximized : 1;

    void set_enabled(MirWindowState state, bool enabled);
};
}
}

#endif // MIR_SCENE_SURFACE_STATE_TRACKER_
