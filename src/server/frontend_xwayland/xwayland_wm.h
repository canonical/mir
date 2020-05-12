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

#ifndef MIR_FRONTEND_XWAYLAND_WM_H
#define MIR_FRONTEND_XWAYLAND_WM_H

#include "mir/dispatch/threaded_dispatcher.h"
#include "wayland_connector.h"
#include "xcb_connection.h"

#include <map>
#include <thread>
#include <experimental/optional>
#include <mutex>

#include <wayland-server-core.h>

#include <X11/Xcursor/Xcursor.h>
#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>

namespace mir
{
namespace dispatch
{
class ReadableFd;
class ThreadedDispatcher;
class MultiplexingDispatchable;
}
namespace frontend
{
class XWaylandSurface;
class XWaylandWMShell;

template<typename T, size_t length>
constexpr size_t length_of(T(&)[length])
{
    return length;
}

class XWaylandWM
{
private:
    std::shared_ptr<XCBConnection> const connection;

public:
    /// Takes ownership of the given FD
    XWaylandWM(std::shared_ptr<WaylandConnector> wayland_connector, wl_client* wayland_client, int fd);
    ~XWaylandWM();

    auto get_wm_surface(xcb_window_t xcb_window) -> std::experimental::optional<std::shared_ptr<XWaylandSurface>>;
    auto get_focused_window() -> std::experimental::optional<xcb_window_t>;
    void set_focus(xcb_window_t xcb_window, bool should_be_focused);
    void run_on_wayland_thread(std::function<void()>&& work);

private:
    enum CursorType
    {
        CursorUnset = -1,
        CursorTop,
        CursorBottom,
        CursorLeft,
        CursorRight,
        CursorTopLeft,
        CursorTopRight,
        CursorBottomLeft,
        CursorBottomRight,
        CursorLeftPointer
    };

    void create_wm_window();
    void wm_selector();

    void create_window(xcb_window_t id);
    void set_cursor(xcb_window_t id, const CursorType &cursor);
    void create_wm_cursor();
    void wm_get_resources();

    // Event handeling
    void handle_events();
    void handle_event(xcb_generic_event_t* event);

    // Events
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

    std::mutex mutex;

    // Cursor
    xcb_cursor_t xcb_cursor_image_load_cursor(const XcursorImage *img);
    xcb_cursor_t xcb_cursor_images_load_cursor(const XcursorImages *images);
    xcb_cursor_t xcb_cursor_library_load_cursor(const char *file);

    std::shared_ptr<WaylandConnector> const wayland_connector;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const dispatcher;
    wl_client* const wayland_client;
    std::shared_ptr<XWaylandWMShell> const wm_shell;

    xcb_window_t wm_window;
    std::map<xcb_window_t, std::shared_ptr<XWaylandSurface>> surfaces;
    std::experimental::optional<xcb_window_t> focused_window;
    std::shared_ptr<dispatch::ReadableFd> wm_dispatcher;
    int xcb_cursor;
    std::vector<xcb_cursor_t> xcb_cursors;
    xcb_window_t xcb_selection_window;
    xcb_selection_request_event_t xcb_selection_request;
    xcb_render_pictforminfo_t xcb_format_rgb, xcb_format_rgba;
    const xcb_query_extension_reply_t *xfixes;
    std::unique_ptr<dispatch::ThreadedDispatcher> event_thread;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_H */
