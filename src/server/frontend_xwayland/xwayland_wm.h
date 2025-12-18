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

#ifndef MIR_FRONTEND_XWAYLAND_WM_H
#define MIR_FRONTEND_XWAYLAND_WM_H

#include <mir/dispatch/threaded_dispatcher.h>
#include <mir/geometry/rectangle.h>
#include "wayland_connector.h"
#include "xcb_connection.h"

#include <map>
#include <set>
#include <thread>
#include <optional>
#include <mutex>

#include <wayland-server-core.h>
#include <xcb/xfixes.h>

namespace mir
{
namespace scene
{
using SurfaceSet = std::set<std::weak_ptr<Surface>, std::owner_less<std::weak_ptr<Surface>>>;
class Surface;
}
namespace dispatch
{
class MultiplexingDispatchable;
}
namespace frontend
{
class XWaylandSurface;
class XWaylandWMShell;
class XWaylandCursors;
class XWaylandClientManager;
class XWaylandClipboardSource;
class XWaylandClipboardProvider;

class XWaylandSceneObserver;

class XWaylandWM
{
private:
    std::shared_ptr<XCBConnection> const connection;

public:
    /// Takes ownership of the given FD
    XWaylandWM(
        std::shared_ptr<WaylandConnector> wayland_connector,
        wl_client* wayland_client,
        Fd const& fd,
        std::shared_ptr<dispatch::MultiplexingDispatchable> const& dispatcher,
        float assumed_surface_scale);
    ~XWaylandWM();

    /// Called by the XWayland connector when there may be new events
    void handle_events();

    auto get_wm_surface(xcb_window_t xcb_window) -> std::optional<std::shared_ptr<XWaylandSurface>>;
    auto get_focused_window() -> std::optional<xcb_window_t>;
    void set_focus(xcb_window_t xcb_window, bool should_be_focused);
    void remember_scene_surface(std::weak_ptr<scene::Surface> const& scene_surface, xcb_window_t window);
    void forget_scene_surface(std::weak_ptr<scene::Surface> const& scene_surface);

    void surfaces_reordered(scene::SurfaceSet const& affected_surfaces);

private:
    XWaylandWM(XWaylandWM const&) = delete;
    XWaylandWM& operator=(XWaylandWM const&) = delete;

    void restack_surfaces();
    auto recalculate_client_list_locked() const -> std::vector<xcb_window_t>;
    auto recalculate_client_stacking_list_locked() const -> std::vector<xcb_window_t>;
    void update_client_list(std::vector<xcb_window_t> const& windows);
    void update_client_list_stacking(std::vector<xcb_window_t> const& windows);

    /// Called for all windows at startup and whenever a window is created
    /// May occasionally be called multiple times for the same window
    void manage_window(xcb_window_t window, geometry::Rectangle const& geometry, bool override_redirect);

    void handle_event(xcb_generic_event_t* event);
    void handle_create_notify(xcb_create_notify_event_t *event);
    void handle_motion_notify(xcb_motion_notify_event_t *event);
    void handle_property_notify(xcb_property_notify_event_t *event);
    void handle_map_request(xcb_map_request_event_t *event);
    void handle_surface_id(std::weak_ptr<XWaylandSurface> const& weak_surface, xcb_client_message_event_t *event);
    void handle_move_resize(std::shared_ptr<XWaylandSurface> surface, xcb_client_message_event_t *event);
    void handle_client_message(xcb_client_message_event_t *event);
    void handle_configure_request(xcb_configure_request_event_t *event);
    void handle_configure_notify(xcb_configure_notify_event_t *event);
    void handle_unmap_notify(xcb_unmap_notify_event_t *event);
    void handle_destroy_notify(xcb_destroy_notify_event_t *event);
    void handle_focus_in(xcb_focus_in_event_t* event);
    void handle_error(xcb_generic_error_t* event);

    xcb_query_extension_reply_t const* const xfixes; ///< Must not be freed, can be null
    std::shared_ptr<WaylandConnector> const wayland_connector;
    wl_client* const wayland_client;
    std::shared_ptr<XWaylandWMShell> const wm_shell;
    Executor& wayland_executor;
    std::unique_ptr<XWaylandCursors> const cursors;
    std::unique_ptr<XWaylandClipboardSource> const clipboard_source;
    std::unique_ptr<XWaylandClipboardProvider> const clipboard_provider;
    xcb_window_t const wm_window;
    std::shared_ptr<XWaylandSceneObserver> const scene_observer;
    std::shared_ptr<XWaylandClientManager> const client_manager;
    /// The scale we assume applications are rendering at. If this doesn't match the scale an app is actually rendering
    /// at the app will appear the wrong size. If this matches the app but both are smaller than the output scale, the
    /// app will appear the correct size but blurry.
    float const assumed_surface_scale;

    std::mutex mutex;
    std::map<xcb_window_t, std::shared_ptr<XWaylandSurface>> surfaces;
    std::map<std::weak_ptr<
        scene::Surface>,
        xcb_window_t,
        std::owner_less<std::weak_ptr<scene::Surface>>> scene_surfaces;
    /// Could be regenerated from scene_surfaces at any time, but more efficient to keep this up to date
    std::set<std::weak_ptr<scene::Surface>, std::owner_less<std::weak_ptr<scene::Surface>>> scene_surface_set;
    std::optional<xcb_window_t> focused_window;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_H */
