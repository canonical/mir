/*
 * Copyright © 2018-2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_WAYLAND_SURFACE_OBSERVER_H_
#define MIR_FRONTEND_WAYLAND_SURFACE_OBSERVER_H_

#include "mir/scene/null_surface_observer.h"

#include <memory>
#include <experimental/optional>
#include <chrono>
#include <functional>

struct wl_client;

namespace mir
{
namespace frontend
{
class WlSurface;
class WlSeat;
class WindowWlSurfaceRole;
class WaylandInputDispatcher;

class WaylandSurfaceObserver
    : public scene::NullSurfaceObserver
{
public:
    WaylandSurfaceObserver(WlSeat* seat, wl_client* client, WlSurface* surface, WindowWlSurfaceRole* window);
    ~WaylandSurfaceObserver();

    /// Overrides from scene::SurfaceObserver
    ///@{
    void attrib_changed(scene::Surface const*, MirWindowAttrib attrib, int value) override;
    void content_resized_to(scene::Surface const*, geometry::Size const& content_size) override;
    void client_surface_close_requested(scene::Surface const*) override;
    void keymap_changed(
        scene::Surface const*,
        MirInputDeviceId id,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options) override;
    void placed_relative(scene::Surface const*, geometry::Rectangle const& placement) override;
    void input_consumed(scene::Surface const*, MirEvent const* event) override;
    ///@}

    void latest_client_size(geometry::Size window_size)
    {
        this->window_size = window_size;
    }

    std::experimental::optional<geometry::Size> requested_window_size()
    {
        return requested_size;
    }

    auto latest_timestamp() const -> std::chrono::nanoseconds;

    auto state() const -> MirWindowState
    {
        return current_state;
    }

    void disconnect() { *destroyed = true; }

private:
    WlSeat* const seat; // only used by run_on_wayland_thread_unless_destroyed()
    WindowWlSurfaceRole* const window;
    std::unique_ptr<WaylandInputDispatcher> const input_dispatcher;

    geometry::Size window_size;
    std::experimental::optional<geometry::Size> requested_size;
    MirWindowState current_state{mir_window_state_unknown};
    std::shared_ptr<bool> const destroyed;

    void run_on_wayland_thread_unless_destroyed(std::function<void()>&& work);
};
}
}

#endif // MIR_FRONTEND_WAYLAND_SURFACE_OBSERVER_H_
