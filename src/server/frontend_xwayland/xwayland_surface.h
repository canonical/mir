/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
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

#include <mutex>
#include <chrono>
#include <set>

extern "C" {
#include <xcb/xcb.h>
}

struct wm_size_hints
{
    uint32_t flags;
    int32_t x, y;
    int32_t width, height; /* should set so old wm's don't mess up */
    int32_t min_width, min_height;
    int32_t max_width, max_height;
    int32_t width_inc, height_inc;
    struct
    {
        int32_t x;
        int32_t y;
    } min_aspect, max_aspect;
    int32_t base_width, base_height;
    int32_t win_gravity;
};

#define USPosition (1L << 0)
#define USSize (1L << 1)
#define PPosition (1L << 2)
#define PSize (1L << 3)
#define PMinSize (1L << 4)
#define PMaxSize (1L << 5)
#define PResizeInc (1L << 6)
#define PAspect (1L << 7)
#define PBaseSize (1L << 8)
#define PWinGravity (1L << 9)

struct motif_wm_hints
{
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
};

#define MWM_HINTS_FUNCTIONS (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_INPUT_MODE (1L << 2)
#define MWM_HINTS_STATUS (1L << 3)

#define MWM_FUNC_ALL (1L << 0)
#define MWM_FUNC_RESIZE (1L << 1)
#define MWM_FUNC_MOVE (1L << 2)
#define MWM_FUNC_MINIMIZE (1L << 3)
#define MWM_FUNC_MAXIMIZE (1L << 4)
#define MWM_FUNC_CLOSE (1L << 5)

#define MWM_DECOR_ALL (1L << 0)
#define MWM_DECOR_BORDER (1L << 1)
#define MWM_DECOR_RESIZEH (1L << 2)
#define MWM_DECOR_TITLE (1L << 3)
#define MWM_DECOR_MENU (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define MWM_DECOR_EVERYTHING \
    (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE)

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3
#define MWM_INPUT_APPLICATION_MODAL MWM_INPUT_PRIMARY_APPLICATION_MODAL

#define MWM_TEAROFF_WINDOW (1L << 0)

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT 0
#define _NET_WM_MOVERESIZE_SIZE_TOP 1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT 2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT 3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT 4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM 5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT 6
#define _NET_WM_MOVERESIZE_SIZE_LEFT 7
#define _NET_WM_MOVERESIZE_MOVE 8           /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD 9  /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD 10 /* move via keyboard */
#define _NET_WM_MOVERESIZE_CANCEL 11        /* cancel operation */

#define TYPE_WM_PROTOCOLS XCB_ATOM_CUT_BUFFER0
#define TYPE_MOTIF_WM_HINTS XCB_ATOM_CUT_BUFFER1
#define TYPE_NET_WM_STATE XCB_ATOM_CUT_BUFFER2
#define TYPE_WM_NORMAL_HINTS XCB_ATOM_CUT_BUFFER3

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
    void wl_surface_committed() override;
    auto scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>> override;
    /// @}

    /// Creates a pending spec if needed and returns a reference
    auto pending_spec(
        std::lock_guard<std::mutex> const&) -> shell::SurfaceSpecification&;

    /// Clears the pending spec and returns what it was
    auto consume_pending_spec(
        std::lock_guard<std::mutex> const&) -> std::experimental::optional<std::unique_ptr<shell::SurfaceSpecification>>;

    /// Should NOT be called under lock
    /// Does nothing if we already have a scene::Surface
    void create_scene_surface_if_needed();

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
    std::experimental::optional<std::unique_ptr<InitialWlSurfaceData>> initial_wl_surface_data;
    std::experimental::optional<std::shared_ptr<XWaylandSurfaceObserver>> surface_observer;
    std::weak_ptr<scene::Session> weak_session;
    std::unique_ptr<shell::SurfaceSpecification> nullable_pending_spec;
    std::weak_ptr<scene::Surface> weak_scene_surface;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H */
