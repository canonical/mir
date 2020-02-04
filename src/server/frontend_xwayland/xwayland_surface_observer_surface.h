/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_FRONTEND_XWAYLAND_SURFACE_OBSERVER_SURFACE_H
#define MIR_FRONTEND_XWAYLAND_SURFACE_OBSERVER_SURFACE_H

#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"

#include <functional>

namespace mir
{
namespace frontend
{
/// The interface used by XWaylandSurfaceObserver to talk to the surface object
class XWaylandSurfaceObserverSurface
{
public:
    virtual void scene_surface_state_set(MirWindowState new_state) = 0;
    virtual void scene_surface_resized(geometry::Size const& new_size) = 0;
    virtual void scene_surface_moved_to(geometry::Point const& new_top_left) = 0;
    virtual void scene_surface_close_requested() = 0;
    virtual void run_on_wayland_thread(std::function<void()>&& work) = 0;

    virtual ~XWaylandSurfaceObserverSurface() = default;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_SURFACE_OBSERVER_SURFACE_H
