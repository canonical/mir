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
#include "xwayland_client_manager.h"
#include "xwayland_surface_role_surface.h"
#include "xwayland_surface_observer_surface.h"
#include "mir/scene/surface_state_tracker.h"
#include "mir/proof_of_mutex_lock.h"

#include <xcb/xcb.h>

#include <mutex>
#include <chrono>
#include <set>
#include <deque>

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
        XWaylandWMShell const& wm_shell,
        std::shared_ptr<XWaylandClientManager> const& client_manager,
        xcb_window_t window,
        geometry::Rectangle const& geometry,
        bool override_redirect,
        float scale);
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
    /// Overrides from XWaylandSurfaceObserverSurface
    /// @{
    void scene_surface_focus_set(bool has_focus) override;
    void scene_surface_state_set(MirWindowState new_state) override;
    void scene_surface_resized(geometry::Size const& new_size) override;
    void scene_surface_moved_to(geometry::Point const& new_top_left) override;
    void scene_surface_close_requested() override;
    /// @}

    /// Overrides from XWaylandSurfaceRoleSurface
    /// Should only be called on the Wayland thread
    /// @{
    void wl_surface_destroyed() override;
    auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> override;
    /// @}

    /// Creates a pending spec if needed and returns a reference
    auto pending_spec(ProofOfMutexLock const&) -> shell::SurfaceSpecification&;

    /// Clears the pending spec and returns what it was
    auto consume_pending_spec(ProofOfMutexLock const&) -> std::optional<std::unique_ptr<shell::SurfaceSpecification>>;

    /// Updates the pending spec
    void is_transient_for(xcb_window_t transient_for);

    /// Updates the window's WM_STATE and _NET_WM_STATE properties. Should NOT be called under lock. Calling with
    /// nullopt withdraws the window.
    void inform_client_of_window_state(std::optional<scene::SurfaceStateTracker> const& state);

    /// Calls connection->configure_window() with the given position and size, as well as tracking the calls made so
    /// future configure notifies can determine if the source was us or the client
    void inform_client_of_geometry(
        std::optional<geometry::X> x,
        std::optional<geometry::Y> y,
        std::optional<geometry::Width> width,
        std::optional<geometry::Height> height);

    /// Requests the scene surface be put into the given state
    /// If the request results in an actual surface state change, the observer will be notified
    /// Should NOT be called under lock
    void request_scene_surface_state(MirWindowState new_state);

    auto latest_input_timestamp(ProofOfMutexLock const&) -> std::chrono::nanoseconds;

    /// Appplies any mods in nullable_pending_spec to the scene_surface (if any)
    void apply_any_mods_to_scene_surface();

    /// Unlike with scaled Wayland surfaces, all the data going to and from the client is in raw pixels. Mir internally
    /// deals with scaled coordinates. This means before modifying the Mir surface we need to scale the values in our
    /// surface spec. We also need to convert from global to local coordinates if the surface has a parent. This
    /// function handles all that.
    void prep_surface_spec(ProofOfMutexLock const&, shell::SurfaceSpecification& mods);

    /// Return data from any surface scaled to XWayland coordinates
    /// @{
    auto scaled_top_left_of(scene::Surface const& surface) -> geometry::Point;
    auto scaled_content_offset_of(scene::Surface const& surface) -> geometry::Displacement;
    auto scaled_content_size_of(scene::Surface const& surface) -> geometry::Size;
    /// @}

    /// Returns a surface that could act as this surface's parent, or nullptr if none
    auto plausible_parent(ProofOfMutexLock const&) -> std::shared_ptr<scene::Surface>;
    /// Applies cached.transient_for and cached.type to the spec
    void apply_cached_transient_for_and_type(ProofOfMutexLock const& lock);
    void wm_size_hints(std::vector<int32_t> const& hints);
    void motif_wm_hints(std::vector<uint32_t> const& hints);

    /// Returns the scene surface associated with a given xcb_window, or nullptr if none
    static auto xcb_window_get_scene_surface(XWaylandWM* xwm, xcb_window_t window) -> std::shared_ptr<scene::Surface>;

    XWaylandWM* const xwm;
    std::shared_ptr<XCBConnection> const connection;
    XWaylandWMShell const& wm_shell;
    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<XWaylandClientManager> const client_manager;
    xcb_window_t const window;
    float const scale;
    std::map<xcb_window_t, std::function<std::function<void()>()>> const property_handlers;

    std::mutex mutable mutex;

    /// Cached version of properties on the X server
    struct
    {
        /// Reflects the _NET_WM_STATE and WM_STATE we have currently set on the window.
        /// Should only be modified by inform_client_of_window_state().
        /// Initially set to hidden, which means withdrawn in X11.
        scene::SurfaceStateTracker state{mir_window_state_restored};

        /// If the window is withdrawn. Should only be modified by inform_client_of_window_state().
        bool withdrawn{true};

        bool override_redirect;

        /// The last geometry we've configured the window with. Note that configure notifications we get do not
        /// immediately change this, as there could be a more recent configure event we've sent still on the wire.
        geometry::Rectangle geometry;

        /// The contents of the _NET_SUPPORTED property set by the client
        std::set<xcb_atom_t> supported_wm_protocols;

        /// True if server-side decorations have been explicitly disabled with motif hints
        bool motif_decorations_disabled{false};

        xcb_window_t transient_for{XCB_WINDOW_NONE};
        MirWindowType type{mir_window_type_freestyle};
    } cached;

    /// When we send a configure we push it to the back, when we get notified of a configure we pop it and all the ones
    /// before it
    std::deque<geometry::Rectangle> inflight_configures;

    /// Set in set_wl_surface and cleared when a scene surface is created from it
    std::optional<std::shared_ptr<XWaylandSurfaceObserver>> surface_observer;
    std::unique_ptr<shell::SurfaceSpecification> nullable_pending_spec;
    std::shared_ptr<XWaylandClientManager::Session> client_session;
    std::weak_ptr<scene::Surface> weak_scene_surface;
    std::weak_ptr<scene::Surface> effective_parent;
};
} /* frontend */
} /* mir */

#endif // MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H
