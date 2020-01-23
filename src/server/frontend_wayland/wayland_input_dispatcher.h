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

#ifndef MIR_FRONTEND_WAYLAND_INPUT_DISPATCHER_H
#define MIR_FRONTEND_WAYLAND_INPUT_DISPATCHER_H

#include "mir_toolkit/common.h"
#include "mir_toolkit/events/event.h"
#include "mir/geometry/point.h"

#include <memory>
#include <chrono>
#include <experimental/optional>

struct wl_client;

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
/// Should only be used from the Wayland thread
class WaylandInputDispatcher
{
public:
    WaylandInputDispatcher(
        WlSeat* seat,
        WlSurface* wl_surface);
    ~WaylandInputDispatcher() = default;

    void set_keymap(input::Keymap const& keymap);
    void set_focus(bool has_focus);
    void handle_event(MirEvent const* event);

    auto latest_timestamp() const -> std::chrono::nanoseconds { return timestamp; }

private:
    WaylandInputDispatcher(WaylandInputDispatcher const&) = delete;
    WaylandInputDispatcher& operator=(WaylandInputDispatcher const&) = delete;

    WlSeat* const seat;
    wl_client* const client;
    WlSurface* const wl_surface;
    std::shared_ptr<bool> const wl_surface_destroyed;

    std::chrono::nanoseconds timestamp{0};
    MirPointerButtons last_pointer_buttons{0};
    std::experimental::optional<geometry::Point> last_pointer_position;

    /// Handle user input events
    ///@{
    void handle_input_event(MirInputEvent const* event);
    void handle_keyboard_event(std::chrono::milliseconds const& ms, MirKeyboardEvent const* event);
    void handle_pointer_event(std::chrono::milliseconds const& ms, MirPointerEvent const* event);
    void handle_pointer_button_event(std::chrono::milliseconds const& ms, MirPointerEvent const* event);
    void handle_pointer_motion_event(std::chrono::milliseconds const& ms, MirPointerEvent const* event);
    void handle_touch_event(std::chrono::milliseconds const& ms, MirTouchEvent const* event);
    ///@}
};
}
}

#endif // MIR_FRONTEND_WAYLAND_INPUT_DISPATCHER_H
