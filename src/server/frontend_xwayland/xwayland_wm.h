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

#include <map>
#include <thread>
#include <wayland-server-core.h>

#include "mir/dispatch/threaded_dispatcher.h"
#include "wayland_connector.h"

extern "C" {
#include <X11/Xcursor/Xcursor.h>
#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>
struct atom_t
{
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_normal_hints;
    xcb_atom_t wm_take_focus;
    xcb_atom_t wm_delete_window;
    xcb_atom_t wm_state;
    xcb_atom_t wm_s0;
    xcb_atom_t wm_client_machine;
    xcb_atom_t net_wm_cm_s0;
    xcb_atom_t net_wm_name;
    xcb_atom_t net_wm_pid;
    xcb_atom_t net_wm_icon;
    xcb_atom_t net_wm_state;
    xcb_atom_t net_wm_state_maximized_vert;
    xcb_atom_t net_wm_state_maximized_horz;
    xcb_atom_t net_wm_state_fullscreen;
    xcb_atom_t net_wm_user_time;
    xcb_atom_t net_wm_icon_name;
    xcb_atom_t net_wm_desktop;
    xcb_atom_t net_wm_window_type;
    xcb_atom_t net_wm_window_type_desktop;
    xcb_atom_t net_wm_window_type_dock;
    xcb_atom_t net_wm_window_type_toolbar;
    xcb_atom_t net_wm_window_type_menu;
    xcb_atom_t net_wm_window_type_utility;
    xcb_atom_t net_wm_window_type_splash;
    xcb_atom_t net_wm_window_type_dialog;
    xcb_atom_t net_wm_window_type_dropdown;
    xcb_atom_t net_wm_window_type_popup;
    xcb_atom_t net_wm_window_type_tooltip;
    xcb_atom_t net_wm_window_type_notification;
    xcb_atom_t net_wm_window_type_combo;
    xcb_atom_t net_wm_window_type_dnd;
    xcb_atom_t net_wm_window_type_normal;
    xcb_atom_t net_wm_moveresize;
    xcb_atom_t net_supporting_wm_check;
    xcb_atom_t net_supported;
    xcb_atom_t net_active_window;
    xcb_atom_t motif_wm_hints;
    xcb_atom_t clipboard;
    xcb_atom_t clipboard_manager;
    xcb_atom_t targets;
    xcb_atom_t utf8_string;
    xcb_atom_t wl_selection;
    xcb_atom_t incr;
    xcb_atom_t timestamp;
    xcb_atom_t multiple;
    xcb_atom_t compound_text;
    xcb_atom_t text;
    xcb_atom_t string;
    xcb_atom_t window;
    xcb_atom_t text_plain_utf8;
    xcb_atom_t text_plain;
    xcb_atom_t xdnd_selection;
    xcb_atom_t xdnd_aware;
    xcb_atom_t xdnd_enter;
    xcb_atom_t xdnd_leave;
    xcb_atom_t xdnd_drop;
    xcb_atom_t xdnd_status;
    xcb_atom_t xdnd_finished;
    xcb_atom_t xdnd_type_list;
    xcb_atom_t xdnd_action_copy;
    xcb_atom_t wl_surface_id;
    xcb_atom_t allow_commits;
};
}

namespace mir
{
namespace dispatch
{
class ReadableFd;
class ThreadedDispatcher;
class MultiplexingDispatchable;
} /*dispatch */
namespace frontend
{
class XWaylandWMSurface;
class XWaylandWMShellSurface;
class WlSurface;

class XWaylandWM
{
public:
    XWaylandWM(std::shared_ptr<WaylandConnector> wayland_connector, wl_client* wayland_client, int fd);
    ~XWaylandWM();

    auto get_xcb_connection() const -> xcb_connection_t*
    {
        return xcb_connection;
    }

    void dump_property(xcb_atom_t property, xcb_get_property_reply_t *reply);
    void set_net_active_window(xcb_window_t window);
    auto build_shell_surface(
        XWaylandWMSurface* wm_surface,
        WlSurface* wayland_surface) -> std::shared_ptr<XWaylandWMShellSurface>;

    atom_t xcb_atom;

private:

    void start();
    void destroy();

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
    void read_and_dump_property(xcb_window_t window, xcb_atom_t property);
    const char *get_atom_name(xcb_atom_t atom);
    bool is_ours(uint32_t id);
    void setup_visual_and_colormap();

    // Event handeling
    void handle_events();

    // Events
    void handle_create_notify(xcb_create_notify_event_t *event);
    void handle_property_notify(xcb_property_notify_event_t *event);
    void handle_map_request(xcb_map_request_event_t *event);
    void handle_change_state(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event);
    void handle_state(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event);
    void handle_surface_id(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event);
    void handle_move_resize(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event);
    void handle_client_message(xcb_client_message_event_t *event);
    void handle_configure_request(xcb_configure_request_event_t *event);
    void handle_unmap_notify(xcb_unmap_notify_event_t *event);
    void handle_destroy_notify(xcb_destroy_notify_event_t *event);

    // Cursor
    xcb_cursor_t xcb_cursor_image_load_cursor(const XcursorImage *img);
    xcb_cursor_t xcb_cursor_images_load_cursor(const XcursorImages *images);
    xcb_cursor_t xcb_cursor_library_load_cursor(const char *file);

    std::shared_ptr<WaylandConnector> const wayland_connector;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const dispatcher;
    wl_client* const wayland_client;
    int const wm_fd;

    xcb_connection_t *xcb_connection;
    xcb_screen_t *xcb_screen;
    xcb_window_t xcb_window;
    std::map<xcb_window_t, std::shared_ptr<XWaylandWMSurface>> surfaces;
    std::shared_ptr<dispatch::ReadableFd> wm_dispatcher;
    int xcb_cursor;
    std::vector<xcb_cursor_t> xcb_cursors;
    xcb_window_t xcb_selection_window;
    xcb_selection_request_event_t xcb_selection_request;
    xcb_render_pictforminfo_t xcb_format_rgb, xcb_format_rgba;
    const xcb_query_extension_reply_t *xfixes;
    std::unique_ptr<dispatch::ThreadedDispatcher> event_thread;
    xcb_visualid_t xcb_visual_id;
    xcb_colormap_t xcb_colormap;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_H */
