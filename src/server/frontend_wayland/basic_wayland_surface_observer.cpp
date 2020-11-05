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

#include "basic_wayland_surface_observer.h"
#include "wl_seat.h"
#include "wayland_utils.h"

namespace mf = mir::frontend;

mf::BasicWaylandSurfaceObserver::BasicWaylandSurfaceObserver(WlSeat* seat) :
    seat{seat},
    destroyed{std::make_shared<bool>(false)}
{
}

mf::BasicWaylandSurfaceObserver::~BasicWaylandSurfaceObserver()
{
    *destroyed = true;
}

void mf::BasicWaylandSurfaceObserver::run_on_wayland_thread_unless_destroyed(std::function<void()>&& work)
{
    seat->spawn(run_unless(destroyed, work));
}
