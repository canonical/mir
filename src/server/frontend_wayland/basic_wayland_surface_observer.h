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
 */

#ifndef MIR_FRONTEND_ABSTRACT_WAYLAND_SURFACE_OBSERVER_H_
#define MIR_FRONTEND_ABSTRACT_WAYLAND_SURFACE_OBSERVER_H_

#include "mir/scene/null_surface_observer.h"

#include <functional>
#include <memory>

namespace mir
{
namespace frontend
{
class WlSeat;

class BasicWaylandSurfaceObserver : public scene::NullSurfaceObserver
{
public:
    BasicWaylandSurfaceObserver(WlSeat* seat);
    ~BasicWaylandSurfaceObserver();

    void disconnect() { *destroyed = true; }

protected:
    void run_on_wayland_thread_unless_destroyed(std::function<void()>&& work);

private:
    WlSeat* const seat; // only used by run_on_wayland_thread_unless_destroyed()
    std::shared_ptr<bool> const destroyed;
};
}
}

#endif // MIR_FRONTEND_ABSTRACT_WAYLAND_SURFACE_OBSERVER_H_
