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
 * Written with alot of help of Weston Xwm
 *
 */

#include "xwayland_wm.h"

#define MIR_LOG_COMPONENT "xwaylandwm"
#include "mir/log.h"
#include "xwayland_log.h"

#include "wl_surface.h"
#include "xwayland_wm_shellsurface.h"
#include "xwayland_wm_surface.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/fd.h"
#include "mir/terminate_with_current_exception.h"

#include <cstring>
#include <poll.h>
#include <sys/socket.h>


#ifndef ARRAY_LENGTH
namespace { template<typename T, size_t size> constexpr size_t length_of(T(&)[size]) {return size;} }
#define ARRAY_LENGTH(a) length_of(a)
#endif

#define CURSOR_ENTRY(x)        \
    {                          \
        (x), ARRAY_LENGTH((x)) \
    }

static const char *bottom_left_corners[] = {"bottom_left_corner", "sw-resize", "size_bdiag"};

static const char *bottom_right_corners[] = {"bottom_right_corner", "se-resize", "size_fdiag"};

static const char *bottom_sides[] = {"bottom_side", "s-resize", "size_ver"};

static const char *left_ptrs[] = {"left_ptr", "default", "top_left_arrow", "left-arrow"};

static const char *left_sides[] = {"left_side", "w-resize", "size_hor"};

static const char *right_sides[] = {"right_side", "e-resize", "size_hor"};

static const char *top_left_corners[] = {"top_left_corner", "nw-resize", "size_fdiag"};

static const char *top_right_corners[] = {"top_right_corner", "ne-resize", "size_bdiag"};

static const char *top_sides[] = {"top_side", "n-resize", "size_ver"};

struct cursor_alternatives
{
    const char **names;
    size_t count;
};

static const struct cursor_alternatives cursors[] = {
    CURSOR_ENTRY(top_sides),           CURSOR_ENTRY(bottom_sides),         CURSOR_ENTRY(left_sides),
    CURSOR_ENTRY(right_sides),         CURSOR_ENTRY(top_left_corners),     CURSOR_ENTRY(top_right_corners),
    CURSOR_ENTRY(bottom_left_corners), CURSOR_ENTRY(bottom_right_corners), CURSOR_ENTRY(left_ptrs)};

namespace mf = mir::frontend;

mf::XWaylandWM::XWaylandWM(std::shared_ptr<mf::WaylandConnector> wc)
    : wlc(wc),
      dispatcher{std::make_shared<mir::dispatch::MultiplexingDispatchable>()},
      xcb_connection(nullptr)
{
}

mf::XWaylandWM::~XWaylandWM()
{
}

void mf::XWaylandWM::destroy() {
  if (event_thread) {
    dispatcher->remove_watch(wm_dispatcher);
    event_thread.reset();
  }

  // xcb_cursors == 2 when its empty
  if (xcb_cursors.size() != 2) {
    mir::log_info("Cleaning cursors");
    for (auto xcb_cursor : xcb_cursors)
      xcb_free_cursor(xcb_connection, xcb_cursor);
  }
  if (xcb_connection != nullptr)
    xcb_disconnect(xcb_connection);
  close(wm_fd);
}

void mf::XWaylandWM::start(wl_client *wlc, const int fd)
{
    uint32_t values[1];
    xcb_atom_t supported[6];

    wlclient = wlc;
    wm_fd = fd;

    xcb_connection = xcb_connect_to_fd(wm_fd, nullptr);
    if (xcb_connection_has_error(xcb_connection))
    {
        mir::log_error("XWAYLAND: xcb_connect_to_fd failed");
        close(wm_fd);
        return;
    }

    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(xcb_connection));
    xcb_screen = iter.data;

    wm_dispatcher =
        std::make_shared<mir::dispatch::ReadableFd>(mir::Fd{mir::IntOwnedFd{wm_fd}}, [this]() { handle_events(); });
    dispatcher->add_watch(wm_dispatcher);

    event_thread = std::make_unique<mir::dispatch::ThreadedDispatcher>(
        "Mir/X11 WM Reader", dispatcher, []() { mir::terminate_with_current_exception(); });

    wm_get_resources();
    setup_visual_and_colormap();

    values[0] =
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes(xcb_connection, xcb_screen->root, XCB_CW_EVENT_MASK, values);

    xcb_composite_redirect_subwindows(xcb_connection, xcb_screen->root, XCB_COMPOSITE_REDIRECT_MANUAL);

    supported[0] = xcb_atom.net_wm_moveresize;
    supported[1] = xcb_atom.net_wm_state;
    supported[2] = xcb_atom.net_wm_state_fullscreen;
    supported[3] = xcb_atom.net_wm_state_maximized_vert;
    supported[4] = xcb_atom.net_wm_state_maximized_horz;
    supported[5] = xcb_atom.net_active_window;
    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_screen->root, xcb_atom.net_supported, XCB_ATOM_ATOM,
                        32, /* format */
                        ARRAY_LENGTH(supported), supported);

    set_net_active_window(XCB_WINDOW_NONE);
    wm_selector();

    xcb_flush(xcb_connection);

    create_wm_cursor();
    set_cursor(xcb_screen->root, CursorLeftPointer);

    create_wm_window();
}

void mf::XWaylandWM::wm_selector()
{
    uint32_t values[1], mask;

    xcb_selection_request.requestor = XCB_NONE;

    values[0] = XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_selection_window = xcb_generate_id(xcb_connection);
    xcb_create_window(xcb_connection, XCB_COPY_FROM_PARENT, xcb_selection_window, xcb_screen->root, 0, 0, 10, 10, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb_screen->root_visual, XCB_CW_EVENT_MASK, values);

    xcb_set_selection_owner(xcb_connection, xcb_selection_window, xcb_atom.clipboard_manager, XCB_TIME_CURRENT_TIME);

    mask = XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
           XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
           XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;
    xcb_xfixes_select_selection_input(xcb_connection, xcb_selection_window, xcb_atom.clipboard, mask);
}

void mf::XWaylandWM::set_cursor(xcb_window_t id, const CursorType &cursor)
{
    if (xcb_cursor == cursor)
        return;

    xcb_cursor = cursor;
    uint32_t cursor_value_list = xcb_cursors[cursor];
    xcb_change_window_attributes(xcb_connection, id, XCB_CW_CURSOR, &cursor_value_list);
    xcb_flush(xcb_connection);
}

void mf::XWaylandWM::create_wm_cursor()
{
    const char *name;
    int count = ARRAY_LENGTH(cursors);

    xcb_cursors.clear();
    xcb_cursors.reserve(count);

    for (int i = 0; i < count; i++)
    {
        for (size_t j = 0; j < cursors[i].count; j++)
        {
            name = cursors[i].names[j];
            xcb_cursors.push_back(xcb_cursor_library_load_cursor(name));
            if (xcb_cursors[i] != static_cast<xcb_cursor_t>(-1))
                break;
        }
    }
}

void mf::XWaylandWM::create_wm_window()
{
    static const char name[] = "Mir XWM";

    xcb_window = xcb_generate_id(xcb_connection);
    xcb_create_window(xcb_connection, XCB_COPY_FROM_PARENT, xcb_window, xcb_screen->root, 0, 0, 10, 10, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb_screen->root_visual, 0, NULL);

    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window, xcb_atom.net_supporting_wm_check,
                        XCB_ATOM_WINDOW, 32, /* format */
                        1, &xcb_window);

    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window, xcb_atom.net_wm_name, xcb_atom.utf8_string,
                        8, /* format */
                        std::strlen(name), name);

    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_screen->root, xcb_atom.net_supporting_wm_check,
                        XCB_ATOM_WINDOW, 32, /* format */
                        1, &xcb_window);

    /* Claim the WM_S0 selection even though we don't support
     * the --replace functionality. */
    xcb_set_selection_owner(xcb_connection, xcb_window, xcb_atom.wm_s0, XCB_TIME_CURRENT_TIME);

    xcb_set_selection_owner(xcb_connection, xcb_window, xcb_atom.net_wm_cm_s0, XCB_TIME_CURRENT_TIME);
}

void mf::XWaylandWM::set_net_active_window(xcb_window_t window)
{
    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_screen->root, xcb_atom.net_active_window,
                        xcb_atom.window, 32, 1, &window);
}

void mf::XWaylandWM::create_window(xcb_window_t id)
{
    surfaces[id] = std::make_shared<XWaylandWMSurface>(this, id);
}

/* Events */
void mf::XWaylandWM::handle_events()
{
    xcb_generic_event_t *event;
    int count = 0;

    while ((event = xcb_poll_for_event(xcb_connection)))
    {
        int type = event->response_type & ~0x80;
        switch (type)
        {
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE:
            mir::log_verbose("XCB_BUTTON_RELEASE");
            //(reinterpret_cast<xcb_button_press_event_t *>(event));
            break;
        case XCB_ENTER_NOTIFY:
            mir::log_verbose("XCB_ENTER_NOTIFY");
            //(reinterpret_cast<xcb_enter_notify_event_t *>(event));
            break;
        case XCB_LEAVE_NOTIFY:
            mir::log_verbose("XCB_LEAVE_NOTIFY");
            //(reinterpret_cast<xcb_leave_notify_event_t *>(event));
            break;
        case XCB_MOTION_NOTIFY:
            mir::log_verbose("XCB_MOTION_NOTIFY");
            //(reinterpret_cast<xcb_motion_notify_event_t *>(event));
            break;
        case XCB_CREATE_NOTIFY:
            mir::log_verbose("XCB_CREATE_NOTIFY");
            handle_create_notify(reinterpret_cast<xcb_create_notify_event_t *>(event));
            break;
        case XCB_MAP_REQUEST:
            mir::log_verbose("XCB_MAP_REQUEST");
            handle_map_request(reinterpret_cast<xcb_map_request_event_t *>(event));
            break;
        case XCB_MAP_NOTIFY:
            mir::log_verbose("XCB_MAP_NOTIFY");
            //(reinterpret_cast<xcb_map_notify_event_t *>(event));
            break;
        case XCB_UNMAP_NOTIFY:
            mir::log_verbose("XCB_UNMAP_NOTIFY");
            handle_unmap_notify(reinterpret_cast<xcb_unmap_notify_event_t *>(event));
            break;
        case XCB_REPARENT_NOTIFY:
            mir::log_verbose("XCB_REPARENT_NOTIFY");
            //(reinterpret_cast<xcb_reparent_notify_event_t *>(event));
            break;
        case XCB_CONFIGURE_REQUEST:
            mir::log_verbose("XCB_CONFIGURE_REQUEST");
            handle_configure_request(reinterpret_cast<xcb_configure_request_event_t *>(event));
            break;
        case XCB_CONFIGURE_NOTIFY:
            mir::log_verbose("XCB_CONFIGURE_NOTIFY");
            //(reinterpret_cast<xcb_configure_notify_event_t *>(event));
            break;
        case XCB_DESTROY_NOTIFY:
            mir::log_verbose("XCB_DESTROY_NOTIFY");
            handle_destroy_notify(reinterpret_cast<xcb_destroy_notify_event_t *>(event));
            break;
        case XCB_MAPPING_NOTIFY:
            mir::log_verbose("XCB_MAPPING_NOTIFY");
            break;
        case XCB_PROPERTY_NOTIFY:
            mir::log_verbose("XCB_PROPERTY_NOTIFY");
            handle_property_notify(reinterpret_cast<xcb_property_notify_event_t *>(event));
            break;
        case XCB_CLIENT_MESSAGE:
            mir::log_verbose("XCB_CLIENT_MESSAGE");
            handle_client_message(reinterpret_cast<xcb_client_message_event_t *>(event));
            break;
        case XCB_FOCUS_IN:
            mir::log_verbose("XCB_FOCUS_IN");
            //(reinterpret_cast<xcb_focus_in_event_t *>(event));
        default:
            break;
        }

        free(event);
        count++;
    }

    if (count > 0)
    {
        xcb_flush(xcb_connection);
    }
}

void mf::XWaylandWM::handle_property_notify(xcb_property_notify_event_t *event)
{
    mir::log_verbose("XCB_PROPERTY_NOTIFY (window %d)", event->window);

    auto surface = surfaces[event->window];
    if (!surface)
        return;

    surface->dirty_properties();

    if (event->state == XCB_PROPERTY_DELETE)
        mir::log_verbose("XCB_PROPERTY_NOTIFY: deleted");
    else
        read_and_dump_property(event->window, event->atom);
}

void mf::XWaylandWM::handle_create_notify(xcb_create_notify_event_t *event)
{
    mir::log_verbose("XCB_CREATE_NOTIFY (window %d, at (%d, %d), width %d, height %d %s)", event->window, event->x,
                     event->y, event->width, event->height, event->override_redirect ? ", override" : "");
    create_window(event->window);
}

void mf::XWaylandWM::handle_destroy_notify(xcb_destroy_notify_event_t *event)
{
    mir::log_verbose("XCB_DESTROY_NOTIFY (window %d, event %d%s)", event->window, event->event,
                     is_ours(event->window) ? ", ours" : "");

    if (is_ours(event->window))
        return;

    if (surfaces.find(event->window) == surfaces.end())
        return;

    surfaces.erase(event->window);
}

void mf::XWaylandWM::handle_map_request(xcb_map_request_event_t *event)
{
    if (is_ours(event->window))
    {
        mir::log_verbose("XCB_MAP_REQUEST (window %d, ours)", event->window);
        return;
    }

    if (surfaces.find(event->window) == surfaces.end())
        return;

    auto surface = surfaces[event->window];

    mir::log_verbose("XCB_MAP_REQUEST (window %d)", event->window);

    surface->read_properties();
    surface->set_wm_state(XWaylandWMSurface::NormalState);
    surface->set_net_wm_state();
    surface->set_workspace(0);
    xcb_map_window(xcb_connection, event->window);
    xcb_flush(xcb_connection);
}

void mf::XWaylandWM::handle_unmap_notify(xcb_unmap_notify_event_t *event)
{
    mir::log_verbose("XCB_UNMAP_NOTIFY (window %d, event %d%s)", event->window, event->event,
                     is_ours(event->window) ? ", ours" : "");

    if (is_ours(event->window))
        return;

    // We just ignore the ICCCM 4.1.4 synthetic unmap notify
    // as it may come in after we've destroyed the window
    if (event->response_type & ~0x80)
        return;

    if (surfaces.find(event->window) == surfaces.end())
        return;

    auto surface = surfaces[event->window];

    surface->set_surface_id(0);
    surface->set_wm_state(XWaylandWMSurface::WithdrawnState);
    surface->set_workspace(-1);
    xcb_unmap_window(xcb_connection, event->window);
    xcb_flush(xcb_connection);
}

void mf::XWaylandWM::handle_client_message(xcb_client_message_event_t *event)
{
    mir::log_verbose("XCB_CLIENT_MESSAGE (%s %d %d %d %d %d win %d)",
                     get_atom_name(event->type),
                     event->data.data32[0],
                     event->data.data32[1],
                     event->data.data32[2],
                     event->data.data32[3],
                     event->data.data32[4],
                     event->window);

    if (surfaces.find(event->window) == surfaces.end())
        return;

    auto surface = surfaces[event->window];

    if (event->type == xcb_atom.net_wm_moveresize)
        handle_move_resize(surface, event);
    else if (event->type == xcb_atom.net_wm_state)
        handle_state(surface, event);
    else if (event->type == xcb_atom.wl_surface_id)
        handle_surface_id(surface, event);
}

void mf::XWaylandWM::handle_move_resize(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event)
{
    if (!surface || !event)
        return;

    mir::log_verbose("handle move resize");

    int detail = event->data.data32[2];
    surface->move_resize(detail);
}

void mf::XWaylandWM::handle_change_state(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event)
{
    if (!surface || !event)
        return;
    mir::log_verbose("Handle change state");

    if (event->data.data32[0] == 3)
        surface->get_shell_surface()->set_state_now(mir_window_state_minimized);
}

void mf::XWaylandWM::handle_state(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event)
{
    if (!surface || !event)
        return;
    mir::log_verbose("Handle state");
}

void mf::XWaylandWM::handle_surface_id(std::shared_ptr<XWaylandWMSurface> surface, xcb_client_message_event_t *event)
{
    if (!surface || !event)
        return;
    mir::log_verbose("handle surface_id");

    if (surface->has_surface())
    {
        mir::log_verbose("Window already has a surface id");
        return;
    }

    uint32_t id = event->data.data32[0];
    surface->set_surface_id(id);

    wl_resource *resource = wl_client_get_object(wlclient, id);
    auto *wlsurface = resource ? WlSurface::from(resource) : nullptr;
    if (wlsurface)
        surface->set_surface(wlsurface);

    // TODO: handle unpaired surfaces!
}

void mf::XWaylandWM::handle_configure_request(xcb_configure_request_event_t *event)
{
    mir::log_verbose("XCB_CONFIGURE_REQUEST (window %d) %d,%d @ %dx%d", event->window, event->x, event->y, event->width,
                     event->height);

    auto shellSurface = surfaces[event->window];

    uint32_t values[6];
    int i = -1;

    if (event->value_mask & XCB_CONFIG_WINDOW_X)
    {
        values[++i] = event->x;
        if (shellSurface && shellSurface->has_surface())
        {
        }
    }
    if (event->value_mask & XCB_CONFIG_WINDOW_Y)
    {
        values[++i] = event->y;
        if (shellSurface && shellSurface->has_surface())
        {
        }
    }

    if (event->value_mask & XCB_CONFIG_WINDOW_WIDTH)
    {
        values[++i] = event->width;
        if (shellSurface && shellSurface->has_surface())
        {
        }
    }
    if (event->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
    {
        values[++i] = event->height;
        if (shellSurface && shellSurface->has_surface())
        {
        }
    }

    if (event->value_mask & XCB_CONFIG_WINDOW_SIBLING)
        values[++i] = event->sibling;

    if (event->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
        values[++i] = event->stack_mode;

    if (i >= 0)
    {
        xcb_configure_window(xcb_connection, event->window, event->value_mask, values);
        xcb_flush(xcb_connection);
    }
}

// Cursor
xcb_cursor_t mf::XWaylandWM::xcb_cursor_image_load_cursor(const XcursorImage *img)
{
    xcb_connection_t *c = xcb_connection;
    xcb_screen_iterator_t s = xcb_setup_roots_iterator(xcb_get_setup(c));
    xcb_screen_t *screen = s.data;
    xcb_gcontext_t gc;
    xcb_pixmap_t pix;
    xcb_render_picture_t pic;
    xcb_cursor_t cursor;
    int stride = img->width * 4;

    pix = xcb_generate_id(c);
    xcb_create_pixmap(c, 32, pix, screen->root, img->width, img->height);

    pic = xcb_generate_id(c);
    xcb_render_create_picture(c, pic, pix, xcb_format_rgba.id, 0, 0);

    gc = xcb_generate_id(c);
    xcb_create_gc(c, gc, pix, 0, 0);

    xcb_put_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, pix, gc, img->width, img->height, 0, 0, 0, 32, stride * img->height,
                  (uint8_t *)img->pixels);
    xcb_free_gc(c, gc);

    cursor = xcb_generate_id(c);
    xcb_render_create_cursor(c, cursor, pic, img->xhot, img->yhot);

    xcb_render_free_picture(c, pic);
    xcb_free_pixmap(c, pix);

    return cursor;
}

xcb_cursor_t mf::XWaylandWM::xcb_cursor_images_load_cursor(const XcursorImages *images)
{
    if (images->nimage != 1)
        return -1;

    return xcb_cursor_image_load_cursor(images->images[0]);
}

xcb_cursor_t mf::XWaylandWM::xcb_cursor_library_load_cursor(const char *file)
{
    xcb_cursor_t cursor;
    XcursorImages *images;
    char *v = NULL;
    int size = 0;

    if (!file)
        return 0;

    v = getenv("XCURSOR_SIZE");
    if (v)
        size = atoi(v);

    if (!size)
        size = 32;

    images = XcursorLibraryLoadImages(file, NULL, size);
    if (!images)
        return -1;

    cursor = xcb_cursor_images_load_cursor(images);
    XcursorImagesDestroy(images);

    return cursor;
}

void mf::XWaylandWM::wm_get_resources()
{

#define F(field) offsetof(struct atom_t, field)

    static const struct
    {
        const char *name;
        int offset;
    } atoms[] = {{"WM_PROTOCOLS", F(wm_protocols)},
                 {"WM_NORMAL_HINTS", F(wm_normal_hints)},
                 {"WM_TAKE_FOCUS", F(wm_take_focus)},
                 {"WM_DELETE_WINDOW", F(wm_delete_window)},
                 {"WM_STATE", F(wm_state)},
                 {"WM_S0", F(wm_s0)},
                 {"WM_CLIENT_MACHINE", F(wm_client_machine)},
                 {"_NET_WM_CM_S0", F(net_wm_cm_s0)},
                 {"_NET_WM_NAME", F(net_wm_name)},
                 {"_NET_WM_PID", F(net_wm_pid)},
                 {"_NET_WM_ICON", F(net_wm_icon)},
                 {"_NET_WM_STATE", F(net_wm_state)},
                 {"_NET_WM_STATE_MAXIMIZED_VERT", F(net_wm_state_maximized_vert)},
                 {"_NET_WM_STATE_MAXIMIZED_HORZ", F(net_wm_state_maximized_horz)},
                 {"_NET_WM_STATE_FULLSCREEN", F(net_wm_state_fullscreen)},
                 {"_NET_WM_USER_TIME", F(net_wm_user_time)},
                 {"_NET_WM_ICON_NAME", F(net_wm_icon_name)},
                 {"_NET_WM_DESKTOP", F(net_wm_desktop)},
                 {"_NET_WM_WINDOW_TYPE", F(net_wm_window_type)},

                 {"_NET_WM_WINDOW_TYPE_DESKTOP", F(net_wm_window_type_desktop)},
                 {"_NET_WM_WINDOW_TYPE_DOCK", F(net_wm_window_type_dock)},
                 {"_NET_WM_WINDOW_TYPE_TOOLBAR", F(net_wm_window_type_toolbar)},
                 {"_NET_WM_WINDOW_TYPE_MENU", F(net_wm_window_type_menu)},
                 {"_NET_WM_WINDOW_TYPE_UTILITY", F(net_wm_window_type_utility)},
                 {"_NET_WM_WINDOW_TYPE_SPLASH", F(net_wm_window_type_splash)},
                 {"_NET_WM_WINDOW_TYPE_DIALOG", F(net_wm_window_type_dialog)},
                 {"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", F(net_wm_window_type_dropdown)},
                 {"_NET_WM_WINDOW_TYPE_POPUP_MENU", F(net_wm_window_type_popup)},
                 {"_NET_WM_WINDOW_TYPE_TOOLTIP", F(net_wm_window_type_tooltip)},
                 {"_NET_WM_WINDOW_TYPE_NOTIFICATION", F(net_wm_window_type_notification)},
                 {"_NET_WM_WINDOW_TYPE_COMBO", F(net_wm_window_type_combo)},
                 {"_NET_WM_WINDOW_TYPE_DND", F(net_wm_window_type_dnd)},
                 {"_NET_WM_WINDOW_TYPE_NORMAL", F(net_wm_window_type_normal)},

                 {"_NET_WM_MOVERESIZE", F(net_wm_moveresize)},
                 {"_NET_SUPPORTING_WM_CHECK", F(net_supporting_wm_check)},
                 {"_NET_SUPPORTED", F(net_supported)},
                 {"_NET_ACTIVE_WINDOW", F(net_active_window)},
                 {"_MOTIF_WM_HINTS", F(motif_wm_hints)},
                 {"CLIPBOARD", F(clipboard)},
                 {"CLIPBOARD_MANAGER", F(clipboard_manager)},
                 {"TARGETS", F(targets)},
                 {"UTF8_STRING", F(utf8_string)},
                 {"_WL_SELECTION", F(wl_selection)},
                 {"INCR", F(incr)},
                 {"TIMESTAMP", F(timestamp)},
                 {"MULTIPLE", F(multiple)},
                 {"UTF8_STRING", F(utf8_string)},
                 {"COMPOUND_TEXT", F(compound_text)},
                 {"TEXT", F(text)},
                 {"STRING", F(string)},
                 {"WINDOW", F(window)},
                 {"text/plain;charset=utf-8", F(text_plain_utf8)},
                 {"text/plain", F(text_plain)},
                 {"XdndSelection", F(xdnd_selection)},
                 {"XdndAware", F(xdnd_aware)},
                 {"XdndEnter", F(xdnd_enter)},
                 {"XdndLeave", F(xdnd_leave)},
                 {"XdndDrop", F(xdnd_drop)},
                 {"XdndStatus", F(xdnd_status)},
                 {"XdndFinished", F(xdnd_finished)},
                 {"XdndTypeList", F(xdnd_type_list)},
                 {"XdndActionCopy", F(xdnd_action_copy)},
                 {"_XWAYLAND_ALLOW_COMMITS", F(allow_commits)},
                 {"WL_SURFACE_ID", F(wl_surface_id)}};
#undef F

    xcb_xfixes_query_version_cookie_t xfixes_cookie;
    xcb_xfixes_query_version_reply_t *xfixes_reply;
    xcb_intern_atom_cookie_t cookies[ARRAY_LENGTH(atoms)];
    xcb_intern_atom_reply_t *reply;
    xcb_render_query_pict_formats_reply_t *formats_reply;
    xcb_render_query_pict_formats_cookie_t formats_cookie;
    xcb_render_pictforminfo_t *formats;
    uint32_t i;

    xcb_prefetch_extension_data(xcb_connection, &xcb_xfixes_id);
    xcb_prefetch_extension_data(xcb_connection, &xcb_composite_id);

    formats_cookie = xcb_render_query_pict_formats(xcb_connection);

    for (i = 0; i < ARRAY_LENGTH(atoms); i++)
        cookies[i] = xcb_intern_atom(xcb_connection, 0, strlen(atoms[i].name), atoms[i].name);

    for (i = 0; i < ARRAY_LENGTH(atoms); i++)
    {
        reply = xcb_intern_atom_reply(xcb_connection, cookies[i], NULL);
        *(xcb_atom_t *)((char *)&xcb_atom + atoms[i].offset) = reply->atom;
        free(reply);
    }

    xfixes = xcb_get_extension_data(xcb_connection, &xcb_xfixes_id);
    if (!xfixes || !xfixes->present)
        mir::log_warning("xfixes not available");

    xfixes_cookie = xcb_xfixes_query_version(xcb_connection, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
    xfixes_reply = xcb_xfixes_query_version_reply(xcb_connection, xfixes_cookie, NULL);

    mir::log_verbose("xfixes version: %d.%d", xfixes_reply->major_version, xfixes_reply->minor_version);

    free(xfixes_reply);

    formats_reply = xcb_render_query_pict_formats_reply(xcb_connection, formats_cookie, 0);
    if (formats_reply == NULL)
        return;

    formats = xcb_render_query_pict_formats_formats(formats_reply);
    for (i = 0; i < formats_reply->num_formats; i++)
    {
        if (formats[i].direct.red_mask != 0xff && formats[i].direct.red_shift != 16)
            continue;
        if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 24)
            xcb_format_rgb = formats[i];
        if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 32 &&
            formats[i].direct.alpha_mask == 0xff && formats[i].direct.alpha_shift == 24)
            xcb_format_rgba = formats[i];
    }

    free(formats_reply);
}

void mf::XWaylandWM::read_and_dump_property(xcb_window_t window, xcb_atom_t property)
{
    xcb_get_property_reply_t *reply;
    xcb_get_property_cookie_t cookie;

    cookie = xcb_get_property(xcb_connection, 0, window, property, XCB_ATOM_ANY, 0, 2048);
    reply = xcb_get_property_reply(xcb_connection, cookie, NULL);

    dump_property(property, reply);

    free(reply);
}

void mf::XWaylandWM::dump_property(xcb_atom_t property, xcb_get_property_reply_t *reply)
{
    int32_t *incr_value;
    const char *text_value, *name;
    xcb_atom_t *atom_value;
    int len;
    uint32_t i;

    mir::log_verbose("prop name %s: ", get_atom_name(property));
    if (reply == NULL)
    {
        mir::log_verbose("(no reply)\n");
        return;
    }

    mir::log_verbose("%s/%d, length %d (value_len %d): ",
                     get_atom_name(reply->type),
                     reply->format,
                     xcb_get_property_value_length(reply),
                     reply->value_len);

    if (reply->type == xcb_atom.incr)
    {
        incr_value = (int32_t *)xcb_get_property_value(reply);
        mir::log_verbose("%d\n", *incr_value);
    }
    else if (reply->type == xcb_atom.utf8_string || reply->type == xcb_atom.string)
    {
        text_value = (const char *)xcb_get_property_value(reply);
        if (reply->value_len > 40)
            len = 40;
        else
            len = reply->value_len;
        mir::log_verbose("\"%.*s\"\n", len, text_value);
    }
    else if (reply->type == XCB_ATOM_ATOM)
    {
        atom_value = (xcb_atom_t *)xcb_get_property_value(reply);
        for (i = 0; i < reply->value_len; i++)
        {
            name = get_atom_name(atom_value[i]);

            mir::log_verbose("name: %s", name);
        }
    }
}

const char *mf::XWaylandWM::get_atom_name(xcb_atom_t atom)
{
    xcb_get_atom_name_cookie_t cookie;
    xcb_get_atom_name_reply_t *reply;
    xcb_generic_error_t *e;
    static char buffer[64];

    if (atom == XCB_ATOM_NONE)
        return "None";

    cookie = xcb_get_atom_name(xcb_connection, atom);
    reply = xcb_get_atom_name_reply(xcb_connection, cookie, &e);

    if (reply)
    {
        snprintf(buffer, sizeof buffer, "%.*s", xcb_get_atom_name_name_length(reply), xcb_get_atom_name_name(reply));
    }
    else
    {
        snprintf(buffer, sizeof buffer, "(atom %u)", atom);
    }

    free(reply);

    return buffer;
}

void mf::XWaylandWM::setup_visual_and_colormap()
{
    xcb_depth_iterator_t depthIterator = xcb_screen_allowed_depths_iterator(xcb_screen);
    xcb_visualtype_t *visualType = nullptr;
    xcb_visualtype_iterator_t visualTypeIterator;
    while (depthIterator.rem > 0)
    {
        if (depthIterator.data->depth == 32)
        {
            visualTypeIterator = xcb_depth_visuals_iterator(depthIterator.data);
            visualType = visualTypeIterator.data;
            break;
        }

        xcb_depth_next(&depthIterator);
    }

    if (!visualType)
    {
        mir::log_warning("No 32-bit visualtype");
        return;
    }

    xcb_visual_id = visualType->visual_id;
    xcb_colormap = xcb_generate_id(xcb_connection);
    xcb_create_colormap(xcb_connection, XCB_COLORMAP_ALLOC_NONE, xcb_colormap, xcb_screen->root, xcb_visual_id);
}

bool mf::XWaylandWM::is_ours(uint32_t id)
{
    auto setup = xcb_get_setup(xcb_connection);
    return (id & ~setup->resource_id_mask) == setup->resource_id_base;
}

// getters

xcb_connection_t *mf::XWaylandWM::get_xcb_connection()
{
    return xcb_connection;
}

atom_t *mf::XWaylandWM::get_xcb_atom()
{
    return &xcb_atom;
}
