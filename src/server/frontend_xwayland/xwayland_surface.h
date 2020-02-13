/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2020 Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H
#define MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H

#include "wl_surface.h"
#include "xwayland_wm.h"
#include "xwayland_surface_role_surface.h"
#include "xwayland_surface_observer_surface.h"

#include <xcb/xcb.h>

#include <mutex>
#include <chrono>
#include <set>

namespace mir
{
namespace shell
{
class Shell;
class SurfaceSpecification;
}
namespace frontend
{
class WlSeat;
class XWaylandWM;
class XWaylandSurfaceObserver;

class XWaylandSurface
    : public XWaylandSurfaceRoleSurface,
      public XWaylandSurfaceObserverSurface
{
public:
    XWaylandSurface(
        XWaylandWM *wm,
        std::shared_ptr<XCBConnection> const& connection,
        WlSeat& seat,
        std::shared_ptr<shell::Shell> const& shell,
        xcb_create_notify_event_t *event);
    ~XWaylandSurface();

    void map();
    void close(); ///< Idempotent
    void take_focus();
    void configure_request(xcb_configure_request_event_t* event);
    void configure_notify(xcb_configure_notify_event_t* event);
    void net_wm_state_client_message(uint32_t const (&data)[5]);
    void wm_change_state_client_message(uint32_t const (&data)[5]);
    void property_notify(xcb_atom_t property);
    void attach_wl_surface(WlSurface* wl_surface); ///< Should only be called on the Wayland thread
    void move_resize(uint32_t detail);

private:
    /// contains more information than just a MirWindowState
    /// (for example if a minimized window would otherwise be maximized)
    struct WindowState
    {
        bool withdrawn{true};
        bool minimized{false};
        bool maximized{false};
        bool fullscreen{false};

        auto operator==(WindowState const& that) const -> bool;
        auto mir_window_state() const -> MirWindowState;
        auto updated_from(MirWindowState state) const -> WindowState; ///< Does not change original
    };

    struct InitialWlSurfaceData;

    /// Overrides from XWaylandSurfaceObserverSurface
    /// @{
    void scene_surface_focus_set(bool has_focus) override;
    void scene_surface_state_set(MirWindowState new_state) override;
    void scene_surface_resized(geometry::Size const& new_size) override;
    void scene_surface_moved_to(geometry::Point const& new_top_left) override;
    void scene_surface_close_requested() override;
    void run_on_wayland_thread(std::function<void()>&& work) override;
    /// @}

    /// Overrides from XWaylandSurfaceRoleSurface
    /// Should only be called on the Wayland thread
    /// @{
    void wl_surface_destroyed() override;
    auto scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>> override;
    /// @}

    /// Creates a pending spec if needed and returns a reference
    auto pending_spec(
        std::lock_guard<std::mutex> const&) -> shell::SurfaceSpecification&;

    /// Clears the pending spec and returns what it was
    auto consume_pending_spec(
        std::lock_guard<std::mutex> const&) -> std::experimental::optional<std::unique_ptr<shell::SurfaceSpecification>>;

    /// Updates the window's WM_STATE and _NET_WM_STATE properties
    /// Should NOT be called under lock
    void inform_client_of_window_state(WindowState const& state);

    /// Requests the scene surface be put into the given state
    /// If the request results in an actual surface state change, the observer will be notified
    /// Should NOT be called under lock
    void request_scene_surface_state(MirWindowState new_state);

    auto latest_input_timestamp(std::lock_guard<std::mutex> const&) -> std::chrono::nanoseconds;

    XWaylandWM* const xwm;
    std::shared_ptr<XCBConnection> const connection;
    WlSeat& seat;
    std::shared_ptr<shell::Shell> const shell;
    xcb_window_t const window;
    std::map<xcb_window_t, std::function<std::function<void()>()>> const property_handlers;

    std::mutex mutable mutex;

    /// Cached version of properties on the X server
    struct
    {
        /// Reflects the _NET_WM_STATE and WM_STATE we have currently set on the window
        /// Should only be modified by set_wm_state()
        WindowState state;

        bool override_redirect;

        geometry::Size size;
        geometry::Point top_left; ///< Always in global coordinates

        /// The contents of the _NET_SUPPORTED property set by the client
        std::set<xcb_atom_t> supported_wm_protocols;
    } cached;

    /// Set in set_wl_surface and cleared when a scene surface is created from it
    std::experimental::optional<std::shared_ptr<XWaylandSurfaceObserver>> surface_observer;
    std::weak_ptr<scene::Session> weak_session;
    std::unique_ptr<shell::SurfaceSpecification> nullable_pending_spec;
    std::weak_ptr<scene::Surface> weak_scene_surface;
};
} /* frontend */
} /* mir */

#endif // MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H
