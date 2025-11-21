/*
 * Copyright (C) Marius Gripsgard <marius@ubports.com>
 * Copyright (C) Canonical Ltd.
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
#include <mir/events/input_event.h>
#include <mir/scene/surface_state_tracker.h>
#include <mir/proof_of_mutex_lock.h>

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

/// See ICCCM 4.1.3.1 (https://tronche.com/gui/x/icccm/sec-4.html)
enum class WmState: uint32_t
{
    WITHDRAWN = 0,
    NORMAL = 1,
    ICONIC = 3,
};

// See https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45805407959456
enum class NetWmStateAction: uint32_t
{
    REMOVE = 0,
    ADD = 1,
    TOGGLE = 2,
};

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
    struct StateTracker
    {
        static auto make_withdrawn() -> StateTracker
        {
            return StateTracker{scene::SurfaceStateTracker(mir_window_state_restored), true};
        }

        auto operator==(StateTracker other) const -> bool
        {
            return mir_state == other.mir_state && withdrawn == other.withdrawn;
        }

        auto active_mir_state() const -> MirWindowState
        {
            return withdrawn ? mir_window_state_hidden : mir_state.active_state();
        }
        auto with_active_mir_state(MirWindowState state) const
        {
            return StateTracker{mir_state.with_active_state(state), false};
        }
        auto with_withdrawn(bool is_withdrawn) const -> StateTracker { return StateTracker{mir_state, is_withdrawn}; }

        auto wm_state() const -> WmState;
        auto with_wm_state(WmState wm_state) const -> StateTracker;

        auto net_wm_state(XCBConnection const& conn) const -> std::optional<std::vector<xcb_atom_t>>;
        auto with_net_wm_state_change(
            XCBConnection const& conn,
            NetWmStateAction action,
            xcb_atom_t net_wm_state) const -> StateTracker;

        // Can be freely copied and assigned
        StateTracker(const StateTracker&) = default;
        StateTracker& operator=(const StateTracker&) = default;

    private:
        explicit StateTracker(scene::SurfaceStateTracker mir_state, bool withdrawn)
            : mir_state{mir_state},
              withdrawn{withdrawn}
        {
        }

        scene::SurfaceStateTracker mir_state;
        bool withdrawn;
    };

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
    void has_window_types(std::vector<xcb_atom_t> const& wm_types);

    /// Updates the window's WM_STATE and _NET_WM_STATE properties. Should NOT be called under lock. Calling with
    /// nullopt withdraws the window.
    void inform_client_of_window_state(std::unique_lock<std::mutex> lock, StateTracker new_state);

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
    void wm_hints(std::vector<int32_t> const& hints);
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
        StateTracker state{StateTracker::make_withdrawn()};

        /// If this window needs the server to give it input focus
        bool input_hint{true};

        bool override_redirect;

        /// The last geometry we've configured the window with. Note that configure notifications we get do not
        /// immediately change this, as there could be a more recent configure event we've sent still on the wire.
        geometry::Rectangle geometry;

        /// The contents of the _NET_SUPPORTED property set by the client
        std::set<xcb_atom_t> supported_wm_protocols;

        /// True if server-side decorations have been explicitly disabled with motif hints
        bool motif_decorations_disabled{false};

        xcb_window_t transient_for{XCB_WINDOW_NONE};
        std::vector<xcb_atom_t> wm_types;
    } cached;

    /// When we send a configure we push it to the back, when we get notified of a configure we pop it and all the ones
    /// before it
    std::deque<geometry::Rectangle> inflight_configures;

    /// Manages the XWaylandSurfaceObserver
    class XWaylandSurfaceObserverManager
    {
        std::weak_ptr<scene::Surface> weak_scene_surface;
        std::shared_ptr<XWaylandSurfaceObserver> surface_observer;
    public:
        XWaylandSurfaceObserverManager(
            std::shared_ptr<scene::Surface> scene_surface,
            std::shared_ptr<XWaylandSurfaceObserver> surface_observer);
        XWaylandSurfaceObserverManager();
        XWaylandSurfaceObserverManager(XWaylandSurfaceObserverManager const&) = delete;
        XWaylandSurfaceObserverManager(XWaylandSurfaceObserverManager&&) = default;
        ~XWaylandSurfaceObserverManager();
        auto operator=(XWaylandSurfaceObserverManager const&) -> XWaylandSurfaceObserverManager& = delete;
        auto operator=(XWaylandSurfaceObserverManager&&) -> XWaylandSurfaceObserverManager& = default;

        auto try_get_resize_event() -> std::shared_ptr<MirInputEvent const>;
    } surface_observer_manager;
    std::unique_ptr<shell::SurfaceSpecification> nullable_pending_spec;
    std::shared_ptr<XWaylandClientManager::Session> client_session;
    std::weak_ptr<scene::Surface> weak_scene_surface;
    std::weak_ptr<scene::Surface> effective_parent;
};
} /* frontend */
} /* mir */

#endif // MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H
