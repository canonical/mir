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

#ifndef MIR_FRONTEND_XWAYLAND_SURFACE_OBSERVER_H
#define MIR_FRONTEND_XWAYLAND_SURFACE_OBSERVER_H

#include "mir/scene/null_surface_observer.h"

#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <experimental/optional>

struct wl_client;

namespace mir
{
class Executor;
namespace frontend
{
class WlSeat;
class WlSurface;
class WaylandInputDispatcher;
class XWaylandSurfaceObserverSurface;

/// Must not outlive the XWaylandSurface
class XWaylandSurfaceObserver
    : public scene::NullSurfaceObserver
{
public:
    XWaylandSurfaceObserver(
        WlSeat& seat,
        WlSurface* wl_surface,
        XWaylandSurfaceObserverSurface* wm_surface);
    ~XWaylandSurfaceObserver();

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
    void input_consumed(scene::Surface const*, MirEvent const* event) override;
    ///@}

    /// Can be called from any thread
    auto latest_timestamp() const -> std::chrono::nanoseconds;

private:
    struct ThreadsafeInputDispatcher
    {
        ThreadsafeInputDispatcher(std::unique_ptr<WaylandInputDispatcher> dispatcher);
        ~ThreadsafeInputDispatcher();

        std::mutex mutex;

        // Innter value should only be used from the Wayland thread and while the mutex is locked
        // Can be nullified from any thread as long as the mutex is locked
        std::experimental::optional<std::unique_ptr<WaylandInputDispatcher>> dispatcher;
    };

    XWaylandSurfaceObserverSurface* const wm_surface;
    std::shared_ptr<ThreadsafeInputDispatcher> const input_dispatcher;

    /// Runs work on the Wayland thread if the input dispatcher still exists
    /// Does nothing if the input dispatcher has already been destroyed
    void aquire_input_dispatcher(std::function<void(WaylandInputDispatcher*)>&& work);
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_SURFACE_OBSERVER_H
