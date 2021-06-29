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

#include "wayland_input_dispatcher.h"
#include <mir/scene/null_surface_observer.h>

#include <memory>
#include <experimental/optional>
#include <chrono>
#include <functional>

struct wl_client;

namespace mir
{
class Executor;
namespace frontend
{
class WlSeat;
class WlSurface;
class WindowWlSurfaceRole;
class WaylandInputDispatcher;

class WaylandSurfaceObserver : public scene::NullSurfaceObserver
{
public:
    WaylandSurfaceObserver(Executor& wayland_executor, WlSeat* seat, WlSurface* surface, WindowWlSurfaceRole* window);
    ~WaylandSurfaceObserver();

    /// Overrides from scene::SurfaceObserver
    ///@{
    void attrib_changed(scene::Surface const*, MirWindowAttrib attrib, int value) override;
    void content_resized_to(scene::Surface const*, geometry::Size const& content_size) override;
    void client_surface_close_requested(scene::Surface const*) override;
    void placed_relative(scene::Surface const*, geometry::Rectangle const& placement) override;
    void input_consumed(scene::Surface const*, MirEvent const* event) override;
    ///@}

    /// Should only be called from the Wayland thread
    void latest_client_size(geometry::Size window_size)
    {
        impl->window_size = window_size;
    }

    /// Should only be called from the Wayland thread
    std::experimental::optional<geometry::Size> requested_window_size()
    {
        return impl->requested_size;
    }

    /// Should only be called from the Wayland thread
    auto latest_timestamp() const -> std::chrono::nanoseconds;

    /// Should only be called from the Wayland thread
    auto state() const -> MirWindowState
    {
        return impl->current_state;
    }

private:
    struct Impl
    {
        Impl(
            wayland::Weak<WindowWlSurfaceRole> window,
            std::unique_ptr<WaylandInputDispatcher> input_dispatcher)
            : window{window},
              input_dispatcher{std::move(input_dispatcher)}
        {
        }

        wayland::Weak<WindowWlSurfaceRole> const window;
        std::unique_ptr<WaylandInputDispatcher> const input_dispatcher;

        geometry::Size window_size{};
        std::experimental::optional<geometry::Size> requested_size{};
        MirWindowState current_state{mir_window_state_unknown};
    };

    void run_on_wayland_thread_unless_window_destroyed(
        std::function<void(Impl* impl, WindowWlSurfaceRole* window)>&& work);

    Executor& wayland_executor;
    /// shared_ptr so it can be captured by lambdas and possibly outlive this object
    std::shared_ptr<Impl> const impl;
};
}
}

#endif // MIR_FRONTEND_WAYLAND_SURFACE_OBSERVER_H_
