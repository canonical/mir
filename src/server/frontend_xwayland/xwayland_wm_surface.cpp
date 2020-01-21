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

#include "xwayland_wm_shellsurface.h"
#include "xwayland_wm_surface.h"
#include "xwayland_log.h"

#include <string.h>

#include <experimental/optional>
#include <map>

namespace mf = mir::frontend;

namespace
{
/// See ICCCM 4.1.3.1 (https://tronche.com/gui/x/icccm/sec-4.html)
enum class WmState: uint32_t
{
    WITHDRAWN = 0,
    NORMAL = 1,
    ICONIC = 3,
};

/// See https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#sourceindication
enum class SourceIndication: uint32_t
{
    UNKNOWN = 0,
    APPLICATION = 1,
    PAGER = 2,
};

auto wm_resize_edge_to_mir_resize_edge(uint32_t wm_resize_edge) -> std::experimental::optional<MirResizeEdge>
{
    switch (wm_resize_edge)
    {
    case _NET_WM_MOVERESIZE_SIZE_TOP:           return mir_resize_edge_north;
    case _NET_WM_MOVERESIZE_SIZE_BOTTOM:        return mir_resize_edge_south;
    case _NET_WM_MOVERESIZE_SIZE_LEFT:          return mir_resize_edge_west;
    case _NET_WM_MOVERESIZE_SIZE_TOPLEFT:       return mir_resize_edge_northwest;
    case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:    return mir_resize_edge_southwest;
    case _NET_WM_MOVERESIZE_SIZE_RIGHT:         return mir_resize_edge_east;
    case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT:      return mir_resize_edge_northeast;
    case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:   return mir_resize_edge_southeast;
    default:                                    return std::experimental::nullopt;
    }
}
}

mf::XWaylandWMSurface::XWaylandWMSurface(XWaylandWM *wm, xcb_create_notify_event_t *event)
    : xwm(wm),
      window(event->window),
      props_dirty(true),
      init{
          event->parent,
          {event->x, event->y},
          {event->width, event->height},
          (bool)event->override_redirect},
      shell_surface_destroyed{std::make_shared<bool>(true)}
{
    xcb_get_geometry_cookie_t const geometry_cookie = xcb_get_geometry(xwm->get_xcb_connection(), window);

    uint32_t const values[]{
        XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE};
    xcb_change_window_attributes(xwm->get_xcb_connection(), window, XCB_CW_EVENT_MASK, values);

    xcb_get_geometry_reply_t* const geometry_reply =
        xcb_get_geometry_reply(xwm->get_xcb_connection(), geometry_cookie, nullptr);

    if (!geometry_reply)
        mir::log_error("xcb gemom reply faled");
}

mf::XWaylandWMSurface::~XWaylandWMSurface()
{
    aquire_shell_surface([](auto shell_surface)
        {
            delete shell_surface;
        });
}

void mf::XWaylandWMSurface::net_wm_state_client_message(uint32_t (&data)[5])
{
    // The client is requesting a change in state
    // see https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45390969565536

    enum class NetWmStateClientMessageAction: uint32_t
    {
        REMOVE = 0,
        ADD = 1,
        TOGGLE = 2,
    };

    struct NetWmStateClientMessage
    {
        NetWmStateClientMessageAction const action;
        xcb_atom_t const properties[2];
        SourceIndication const source_indication;
    };

    auto const message = (NetWmStateClientMessage*)data;

    WindowState new_window_state;

    {
        std::lock_guard<std::mutex> lock{mutex};

        new_window_state = window_state;

        for (size_t i = 0; i < length_of(message->properties); i++)
        {
            if (xcb_atom_t const property = message->properties[i]) // if there is only one property, the 2nd is 0
            {
                bool nil{false}, *prop_ptr = &nil;

                if (property == xwm->xcb_atom.net_wm_state_hidden)
                    prop_ptr = &new_window_state.minimized;
                else if (property == xwm->xcb_atom.net_wm_state_maximized_horz) // assume vert is also set
                    prop_ptr = &new_window_state.maximized;
                else if (property == xwm->xcb_atom.net_wm_state_fullscreen)
                    prop_ptr = &new_window_state.fullscreen;

                switch (message->action)
                {
                case NetWmStateClientMessageAction::REMOVE: *prop_ptr = false; break;
                case NetWmStateClientMessageAction::ADD: *prop_ptr = true; break;
                case NetWmStateClientMessageAction::TOGGLE: *prop_ptr = !*prop_ptr; break;
                }
            }
        }
    }

    set_window_state(new_window_state);
}

void mf::XWaylandWMSurface::dirty_properties()
{
    std::lock_guard<std::mutex> lock{mutex};
    props_dirty = true;
}

void mf::XWaylandWMSurface::set_surface(WlSurface* wayland_surface)
{
    auto const shell_surface = xwm->build_shell_surface(this, wayland_surface);
    shell_surface_unsafe = shell_surface;
    shell_surface_destroyed = shell_surface->destroyed_flag();

    {
        std::lock_guard<std::mutex> lock{mutex};
        if (!properties.title.empty())
            shell_surface->set_title(properties.title);
    }

    // If a buffer has alread been committed, we need to create the scene::Surface without waiting for next commit
    if (wayland_surface->buffer_size())
        shell_surface->create_scene_surface();

    xcb_flush(xwm->get_xcb_connection());
}

void mf::XWaylandWMSurface::set_workspace(int workspace)
{
    // Passing a workspace < 0 deletes the property
    if (workspace >= 0)
    {
        xcb_change_property(xwm->get_xcb_connection(), XCB_PROP_MODE_REPLACE, window, xwm->xcb_atom.net_wm_desktop,
                            XCB_ATOM_CARDINAL, 31, 1, &workspace);
    }
    else
    {
        xcb_delete_property(xwm->get_xcb_connection(), window, xwm->xcb_atom.net_wm_desktop);
    }
    xcb_flush(xwm->get_xcb_connection());
}

void mf::XWaylandWMSurface::apply_mir_state_to_window(MirWindowState new_state)
{
    std::vector<xcb_atom_t> net_wm_states;

    WindowState new_window_state;

    {
        std::lock_guard<std::mutex> lock{mutex};

        cached_mir_window_state = new_state;
        new_window_state = window_state;

        switch (new_state)
        {
        case mir_window_state_minimized:
        case mir_window_state_hidden:
            new_window_state.minimized = true;
            // don't change new_window_state.maximized
            // don't change new_window_state.fullscreen
            break;

        case mir_window_state_fullscreen:
            new_window_state.minimized = false;
            // don't change new_window_state.maximized
            new_window_state.fullscreen = true;
            break;

        case mir_window_state_maximized:
        case mir_window_state_vertmaximized:
        case mir_window_state_horizmaximized:
        case mir_window_state_attached:
            new_window_state.minimized = false;
            new_window_state.maximized = true;
            new_window_state.fullscreen = false;
            break;

        case mir_window_state_restored:
        case mir_window_state_unknown:
            new_window_state.minimized = false;
            new_window_state.maximized = false;
            new_window_state.fullscreen = false;
            break;

        case mir_window_states:
            break;
        }
    }

    set_window_state(new_window_state);
}

void mf::XWaylandWMSurface::unmap()
{
    uint32_t const wm_state_properties[]{
        static_cast<uint32_t>(WmState::WITHDRAWN),
        XCB_WINDOW_NONE // Icon window
    };

    xcb_change_property(
        xwm->get_xcb_connection(),
        XCB_PROP_MODE_REPLACE,
        window, xwm->xcb_atom.wm_state,
        xwm->xcb_atom.wm_state, 32,
        length_of(wm_state_properties), wm_state_properties);

    xcb_delete_property(
        xwm->get_xcb_connection(),
        window,
        xwm->xcb_atom.net_wm_state);

    xcb_flush(xwm->get_xcb_connection());
}

void mf::XWaylandWMSurface::read_properties()
{
    std::lock_guard<std::mutex> lock{mutex};

    if (!props_dirty)
        return;
    props_dirty = false;

    std::map<xcb_atom_t, xcb_atom_t> props;
    props[XCB_ATOM_WM_CLASS] = XCB_ATOM_STRING;
    props[XCB_ATOM_WM_NAME] = XCB_ATOM_STRING;
    props[XCB_ATOM_WM_TRANSIENT_FOR] = XCB_ATOM_WINDOW;
    props[xwm->xcb_atom.wm_protocols] = TYPE_WM_PROTOCOLS;
    props[xwm->xcb_atom.wm_normal_hints] = TYPE_WM_NORMAL_HINTS;
    props[xwm->xcb_atom.net_wm_window_type] = XCB_ATOM_ATOM;
    props[xwm->xcb_atom.net_wm_name] = XCB_ATOM_STRING;
    props[xwm->xcb_atom.motif_wm_hints] = TYPE_MOTIF_WM_HINTS;

    std::map<xcb_atom_t, xcb_get_property_cookie_t> cookies;
    for (const auto &atom : props)
    {
        xcb_get_property_cookie_t cookie =
            xcb_get_property(xwm->get_xcb_connection(), 0, window, atom.first, XCB_ATOM_ANY, 0, 2048);
        cookies[atom.first] = cookie;
    }

    properties.deleteWindow = 0;

    for (const auto &atom_ptr : props)
    {
        xcb_atom_t atom = atom_ptr.first;
        xcb_get_property_reply_t *reply = xcb_get_property_reply(xwm->get_xcb_connection(), cookies[atom], nullptr);
        if (!reply)
        {
            // Bad window, usually
            continue;
        }

        if (reply->type == XCB_ATOM_NONE)
        {
            // No such info
            free(reply);
            continue;
        }

        switch (props[atom])
        {
        case XCB_ATOM_STRING:
        {
            char *p = strndup(reinterpret_cast<char *>(xcb_get_property_value(reply)),
                              xcb_get_property_value_length(reply));
            if (atom == XCB_ATOM_WM_CLASS) {
                properties.appId = std::string(p);
            } else if (atom == XCB_ATOM_WM_NAME || xwm->xcb_atom.net_wm_name) {
                properties.title = std::string(p);
            } else {
                free(p);
            }
            break;
        }
        case XCB_ATOM_WINDOW:
        {
            break;
        }
        case XCB_ATOM_ATOM:
        {
            if (atom == xwm->xcb_atom.net_wm_window_type)
            {
            }
            break;
        }
        case TYPE_WM_PROTOCOLS:
        {
            xcb_atom_t *atoms = reinterpret_cast<xcb_atom_t *>(xcb_get_property_value(reply));
            for (uint32_t i = 0; i < reply->value_len; ++i)
                if (atoms[i] == xwm->xcb_atom.wm_delete_window)
                    properties.deleteWindow = 1;
            break;
        }
        case TYPE_WM_NORMAL_HINTS:
        {
            break;
        }
        case TYPE_MOTIF_WM_HINTS:
            break;
        default:
            break;
        }

        free(reply);
    }
}

void mf::XWaylandWMSurface::move_resize(uint32_t detail)
{

    if (detail == _NET_WM_MOVERESIZE_MOVE)
    {
        aquire_shell_surface([](auto shell_surface)
        {
            shell_surface->initiate_interactive_move();
        });
    }
    else if (auto const edge = wm_resize_edge_to_mir_resize_edge(detail))
    {
        aquire_shell_surface([edge](auto shell_surface)
        {
            shell_surface->initiate_interactive_resize(edge.value());
        });
    }
    else
    {
        mir::log_warning("XWaylandWMSurface::move_resize() called with unknown detail %d", detail);
    }
}

void mf::XWaylandWMSurface::send_resize(const geometry::Size& new_size)
{
    uint32_t const mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;

    uint32_t const values[]{
        new_size.width.as_uint32_t(),
        new_size.height.as_uint32_t()};

    xcb_configure_window(xwm->get_xcb_connection(), window, mask, values);
    xcb_flush(xwm->get_xcb_connection());
}

void mf::XWaylandWMSurface::send_close_request()
{
    xcb_destroy_window(xwm->get_xcb_connection(), window);
    xcb_flush(xwm->get_xcb_connection());
}

void mf::XWaylandWMSurface::aquire_shell_surface(std::function<void(XWaylandWMShellSurface* shell_surface)>&& work)
{
    xwm->run_on_wayland_thread(
        [work = move(work), shell_surface = shell_surface_unsafe, destroyed = shell_surface_destroyed]()
        {
            if (!*destroyed)
                work(shell_surface);
        });
}

void mf::XWaylandWMSurface::set_window_state(WindowState const& new_window_state)
{
    WmState const wm_state{
        new_window_state.minimized ?
        WmState::ICONIC :
        WmState::NORMAL};

    uint32_t const wm_state_properties[]{
        static_cast<uint32_t>(wm_state),
        XCB_WINDOW_NONE // Icon window
    };

    xcb_change_property(
        xwm->get_xcb_connection(),
        XCB_PROP_MODE_REPLACE,
        window, xwm->xcb_atom.wm_state,
        xwm->xcb_atom.wm_state, 32,
        length_of(wm_state_properties), wm_state_properties);

    std::vector<xcb_atom_t> net_wm_states;

    if (new_window_state.maximized)
    {
        net_wm_states.push_back(xwm->xcb_atom.net_wm_state_hidden);
    }
    if (new_window_state.maximized)
    {
        net_wm_states.push_back(xwm->xcb_atom.net_wm_state_maximized_horz);
        net_wm_states.push_back(xwm->xcb_atom.net_wm_state_maximized_vert);
    }
    if (new_window_state.fullscreen)
    {
        net_wm_states.push_back(xwm->xcb_atom.net_wm_state_fullscreen);
    }

    xcb_change_property(
        xwm->get_xcb_connection(),
        XCB_PROP_MODE_REPLACE,
        window,
        xwm->xcb_atom.net_wm_state,
        XCB_ATOM_ATOM, 32, // type and format
        net_wm_states.size(), net_wm_states.data());

    xcb_flush(xwm->get_xcb_connection());

    MirWindowState mir_window_state;

    if (new_window_state.minimized)
        mir_window_state = mir_window_state_minimized;
    else if (new_window_state.fullscreen)
        mir_window_state = mir_window_state_fullscreen;
    else if (new_window_state.maximized)
        mir_window_state = mir_window_state_maximized;
    else
        mir_window_state = mir_window_state_restored;

    {
        std::lock_guard<std::mutex> lock{mutex};

        window_state = new_window_state;

        if (mir_window_state != cached_mir_window_state)
        {
            cached_mir_window_state = mir_window_state;
            aquire_shell_surface([mir_window_state](auto shell_surface)
                {
                    shell_surface->set_state_now(mir_window_state);
                });
        }
    }
}
