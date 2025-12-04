/*
 * Copyright © Canonical Ltd.
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

#ifndef MIR_FRONTEND_WAYLAND_INPUT_DISPATCHER_H
#define MIR_FRONTEND_WAYLAND_INPUT_DISPATCHER_H

#include <mir_toolkit/common.h>
#include <mir/geometry/point.h>
#include <mir/wayland/weak.h>

#include <memory>
#include <chrono>

struct wl_client;
struct MirInputEvent;

namespace mir
{
namespace input
{
class Keymap;
}
namespace frontend
{
class WlSeat;
class WlSurface;

/// Dispatches input events to Wayland clients
/// Should only be created and used from the Wayland thread
class WaylandInputDispatcher
{
public:
    WaylandInputDispatcher(
        WlSeat* seat,
        WlSurface* wl_surface);
    ~WaylandInputDispatcher() = default;

    void handle_event(std::shared_ptr<MirInputEvent const> const& event);

private:
    WaylandInputDispatcher(WaylandInputDispatcher const&) = delete;
    WaylandInputDispatcher& operator=(WaylandInputDispatcher const&) = delete;

    WlSeat* const seat;
    wayland::Weak<WlSurface> const wl_surface;
};
}
}

#endif // MIR_FRONTEND_WAYLAND_INPUT_DISPATCHER_H
