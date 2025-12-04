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

#include <mir/scene/surface_state_tracker.h>
#include <mir/fatal.h>

namespace ms = mir::scene;

ms::SurfaceStateTracker::SurfaceStateTracker(MirWindowState initial)
    : SurfaceStateTracker{SurfaceStateTracker{}, initial}
{
}

auto ms::SurfaceStateTracker::active_state() const -> MirWindowState
{
    if (hidden)
    {
        return mir_window_state_hidden;
    }
    else if (minimized)
    {
        return mir_window_state_minimized;
    }
    else if (fullscreen)
    {
        return mir_window_state_fullscreen;
    }
    else if (attached)
    {
        return mir_window_state_attached;
    }
    else if (vert_maximized && horiz_maximized)
    {
        return mir_window_state_maximized;
    }
    else if (vert_maximized)
    {
        return mir_window_state_vertmaximized;
    }
    else if (horiz_maximized)
    {
        return mir_window_state_horizmaximized;
    }
    else
    {
        return mir_window_state_restored;
    }
}

auto ms::SurfaceStateTracker::with_active_state(MirWindowState state) const -> SurfaceStateTracker
{
    return SurfaceStateTracker{*this, state};
}

auto ms::SurfaceStateTracker::has(MirWindowState state) const -> bool
{
    switch (state)
    {
    case mir_window_state_hidden: return hidden;
    case mir_window_state_minimized: return minimized;
    case mir_window_state_fullscreen: return fullscreen;
    case mir_window_state_attached: return attached;
    case mir_window_state_maximized: return horiz_maximized && vert_maximized;
    case mir_window_state_horizmaximized: return horiz_maximized;
    case mir_window_state_vertmaximized: return vert_maximized;
    case mir_window_state_restored: return (!horiz_maximized && !vert_maximized);
    default: return false;
    }
}

auto ms::SurfaceStateTracker::has_any(std::initializer_list<MirWindowState> const& states) const -> bool
{
    for (auto const& state: states)
    {
        if (has(state))
        {
            return true;
        }
    }
    return false;
}

auto ms::SurfaceStateTracker::with(MirWindowState state) const -> SurfaceStateTracker
{
    return SurfaceStateTracker{*this, state, true};
}

auto ms::SurfaceStateTracker::without(MirWindowState state) const -> SurfaceStateTracker
{
    return SurfaceStateTracker{*this, state, false};
}

ms::SurfaceStateTracker::SurfaceStateTracker(SurfaceStateTracker base, MirWindowState active)
    : SurfaceStateTracker{base}
{
    switch (active)
    {
    case mir_window_state_hidden:
        hidden          = true;
        break;

    case mir_window_state_minimized:
        hidden          = false;
        minimized       = true;
        break;

    case mir_window_state_fullscreen:
        hidden          = false;
        minimized       = false;
        fullscreen      = true;
        break;

    case mir_window_state_attached:
        hidden          = false;
        minimized       = false;
        fullscreen      = false;
        attached        = true;
        break;

    case mir_window_state_maximized:
        hidden          = false;
        minimized       = false;
        fullscreen      = false;
        attached        = false;
        horiz_maximized = true;
        vert_maximized  = true;
        break;

    case mir_window_state_horizmaximized:
        hidden          = false;
        minimized       = false;
        fullscreen      = false;
        attached        = false;
        horiz_maximized = true;
        vert_maximized  = false;
        break;

    case mir_window_state_vertmaximized:
        hidden          = false;
        minimized       = false;
        fullscreen      = false;
        attached        = false;
        horiz_maximized = false;
        vert_maximized  = true;
        break;

    case mir_window_state_restored:
        hidden          = false;
        minimized       = false;
        fullscreen      = false;
        attached        = false;
        horiz_maximized = false;
        vert_maximized  = false;
        break;

    case mir_window_state_unknown:
        break;

    default:
        fatal_error("Invalid window state: %d", active);
    }
}

ms::SurfaceStateTracker::SurfaceStateTracker(SurfaceStateTracker base, MirWindowState state, bool present)
    : SurfaceStateTracker{base}
{
    switch (state)
    {
    case mir_window_state_hidden:
        hidden = present;
        break;

    case mir_window_state_minimized:
        minimized = present;
        break;

    case mir_window_state_fullscreen:
        fullscreen = present;
        break;

    case mir_window_state_attached:
        attached = present;
        break;

    case mir_window_state_maximized:
        horiz_maximized = present;
        vert_maximized = present;
        break;

    case mir_window_state_horizmaximized:
        horiz_maximized = present;
        break;

    case mir_window_state_vertmaximized:
        vert_maximized = present;
        break;

    case mir_window_state_restored:
        fatal_error("Sending mir_window_state_restored to SurfaceStateTracker::with()/without() is not allowed");
        break;

    default:
        fatal_error("Invalid window state: %d", state);
    }
}

auto ms::operator==(ms::SurfaceStateTracker const& lhs, ms::SurfaceStateTracker const& rhs) -> bool
{
    return lhs.hidden == rhs.hidden &&
           lhs.minimized == rhs.minimized &&
           lhs.fullscreen == rhs.fullscreen &&
           lhs.attached == rhs.attached &&
           lhs.horiz_maximized == rhs.horiz_maximized &&
           lhs.vert_maximized == rhs.vert_maximized;
}
