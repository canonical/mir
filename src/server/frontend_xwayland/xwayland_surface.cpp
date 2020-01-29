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

#include "xwayland_surface.h"
#include "xwayland_log.h"
#include "xwayland_surface_observer.h"

#include "mir/frontend/wayland.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell.h"

#include "boost/throw_exception.hpp"

#include <string.h>
#include <experimental/optional>
#include <map>

namespace mf = mir::frontend;
namespace geom = mir::geometry;

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

mf::XWaylandSurface::XWaylandSurface(
    XWaylandWM *wm,
    WlSeat& seat,
    std::shared_ptr<shell::Shell> const& shell,
    xcb_connection_t* connection,
    xcb_create_notify_event_t *event)
    : xwm(wm),
      seat(seat),
      shell(shell),
      connection{connection},
      window(event->window),
      props_dirty(true),
      init{
          event->parent,
          {event->x, event->y},
          {event->width, event->height},
          (bool)event->override_redirect}
{
    uint32_t const value = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE;
    xcb_change_window_attributes(connection, window, XCB_CW_EVENT_MASK, &value);
}

mf::XWaylandSurface::~XWaylandSurface()
{
    close();
}

void mf::XWaylandSurface::map()
{
    scene_surface_state_set(mir_window_state_restored); // TODO: use the real window state
}

void mf::XWaylandSurface::close()
{
    std::shared_ptr<scene::Surface> scene_surface;
    std::shared_ptr<XWaylandSurfaceObserver> observer;

    {
        std::lock_guard<std::mutex> lock{mutex};

        scene_surface = weak_scene_surface.lock();
        weak_scene_surface.reset();

        if (surface_observer)
        {
            observer = surface_observer.value();
        }
        surface_observer = std::experimental::nullopt;
    }

    if (scene_surface && observer)
    {
        scene_surface->remove_observer(observer);
    }

    if (scene_surface)
    {
        shell->destroy_surface(scene_surface->session().lock(), scene_surface);
        scene_surface.reset();
        // Someone may still be holding on to the surface somewhere, and that's fine
    }

    if (observer)
    {
        // make sure surface observer is deleted and will not spew any more events
        std::weak_ptr<XWaylandSurfaceObserver> const weak_observer{observer};
        observer.reset();
        if (auto const should_be_dead_observer = weak_observer.lock())
        {
            fatal_error(
                "surface observer should have been deleted, but was not (use count %d)",
                should_be_dead_observer.use_count());
        }
    }
}

void mf::XWaylandSurface::net_wm_state_client_message(uint32_t const (&data)[5])
{
    // The client is requesting a change in state
    // see https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45390969565536

    enum class Action: uint32_t
    {
        REMOVE = 0,
        ADD = 1,
        TOGGLE = 2,
    };

    auto const* pdata = data;
    auto const action = static_cast<Action>(*pdata++);
    xcb_atom_t const properties[2] = { static_cast<xcb_atom_t>(*pdata++),  static_cast<xcb_atom_t>(*pdata++) };
    auto const source_indication = static_cast<SourceIndication>(*pdata++);

    (void)source_indication;

    WindowState new_window_state;

    {
        std::lock_guard<std::mutex> lock{mutex};

        new_window_state = window_state;

        for (xcb_atom_t const property : properties)
        {
            if (property) // if there is only one property, the 2nd is 0
            {
                bool nil{false}, *prop_ptr = &nil;

                if (property == xwm->xcb_atom.net_wm_state_hidden)
                    prop_ptr = &new_window_state.minimized;
                else if (property == xwm->xcb_atom.net_wm_state_maximized_horz) // assume vert is also set
                    prop_ptr = &new_window_state.maximized;
                else if (property == xwm->xcb_atom.net_wm_state_fullscreen)
                    prop_ptr = &new_window_state.fullscreen;

                switch (action)
                {
                case Action::REMOVE: *prop_ptr = false; break;
                case Action::ADD: *prop_ptr = true; break;
                case Action::TOGGLE: *prop_ptr = !*prop_ptr; break;
                }
            }
        }
    }

    set_window_state(new_window_state);
}

void mf::XWaylandSurface::wm_change_state_client_message(uint32_t const (&data)[5])
{
    // See ICCCM 4.1.4 (https://tronche.com/gui/x/icccm/sec-4.html)

    WmState const requested_state = static_cast<WmState>(data[0]);

    WindowState new_window_state;

    {
        std::lock_guard<std::mutex> lock{mutex};

        new_window_state = window_state;

        switch (requested_state)
        {
        case WmState::NORMAL:
            new_window_state.minimized = false;
            break;

        case WmState::ICONIC:
            new_window_state.minimized = true;
            break;

        default:
            break;
        }
    }

    set_window_state(new_window_state);
}

void mf::XWaylandSurface::dirty_properties()
{
    std::lock_guard<std::mutex> lock{mutex};
    props_dirty = true;
}

void mf::XWaylandSurface::set_surface(WlSurface* wl_surface)
{
    // We assume we are on the Wayland thread
    // If a buffer has alread been committed, we need to create the scene::Surface without waiting for next commit
    if (wl_surface->buffer_size())
    {
        create_scene_surface_if_needed(wl_surface);
    }
}

void mf::XWaylandSurface::set_workspace(int workspace)
{
    // Passing a workspace < 0 deletes the property
    if (workspace >= 0)
    {
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, xwm->xcb_atom.net_wm_desktop,
                            XCB_ATOM_CARDINAL, 31, 1, &workspace);
    }
    else
    {
        xcb_delete_property(connection, window, xwm->xcb_atom.net_wm_desktop);
    }
    xcb_flush(connection);
}

void mf::XWaylandSurface::unmap()
{
    uint32_t const wm_state_properties[]{
        static_cast<uint32_t>(WmState::WITHDRAWN),
        XCB_WINDOW_NONE // Icon window
    };

    xcb_change_property(
        connection,
        XCB_PROP_MODE_REPLACE,
        window, xwm->xcb_atom.wm_state,
        xwm->xcb_atom.wm_state, 32,
        length_of(wm_state_properties), wm_state_properties);

    xcb_delete_property(
        connection,
        window,
        xwm->xcb_atom.net_wm_state);

    xcb_flush(connection);
}

void mf::XWaylandSurface::read_properties()
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
            xcb_get_property(connection, 0, window, atom.first, XCB_ATOM_ANY, 0, 2048);
        cookies[atom.first] = cookie;
    }

    properties.deleteWindow = 0;

    for (const auto &atom_ptr : props)
    {
        xcb_atom_t atom = atom_ptr.first;
        xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookies[atom], nullptr);
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

void mf::XWaylandSurface::move_resize(uint32_t detail)
{
    if (detail == _NET_WM_MOVERESIZE_MOVE)
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (auto const scene_surface = weak_scene_surface.lock())
        {
            shell->request_move(
                scene_surface->session().lock(),
                scene_surface,
                latest_input_timestamp(lock).count());
        }
    }
    else if (auto const edge = wm_resize_edge_to_mir_resize_edge(detail))
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (auto const scene_surface = weak_scene_surface.lock())
        {
            shell->request_resize(
                scene_surface->session().lock(),
                scene_surface,
                latest_input_timestamp(lock).count(),
                edge.value());
        }
    }
    else
    {
        mir::log_warning("XWaylandSurface::move_resize() called with unknown detail %d", detail);
    }
}

void mf::XWaylandSurface::scene_surface_state_set(MirWindowState new_state)
{
    WindowState new_window_state;

    {
        std::lock_guard<std::mutex> lock{mutex};

        if (new_state == cached_mir_window_state)
            return;

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
            new_window_state.minimized = false;
            new_window_state.maximized = true;
            new_window_state.fullscreen = false;
            break;

        case mir_window_state_restored:
        case mir_window_state_unknown:
        case mir_window_state_attached:
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

void mf::XWaylandSurface::scene_surface_resized(const geometry::Size& new_size)
{
    uint32_t const mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;

    uint32_t const values[]{
        new_size.width.as_uint32_t(),
        new_size.height.as_uint32_t()};

    xcb_configure_window(connection, window, mask, values);
    xcb_flush(connection);
}

void mf::XWaylandSurface::scene_surface_close_requested()
{
    xcb_destroy_window(connection, window);
    xcb_flush(connection);
}

void mf::XWaylandSurface::run_on_wayland_thread(std::function<void()>&& work)
{
    xwm->run_on_wayland_thread(std::move(work));
}

void mf::XWaylandSurface::wl_surface_destroyed()
{
    close();
}

void mf::XWaylandSurface::wl_surface_committed(WlSurface* wl_surface)
{
    create_scene_surface_if_needed(wl_surface);
}

auto mf::XWaylandSurface::scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>>
{
    std::lock_guard<std::mutex> lock{mutex};
    if (auto const scene_surface = weak_scene_surface.lock())
        return scene_surface;
    else
        return std::experimental::nullopt;
}

void mf::XWaylandSurface::create_scene_surface_if_needed(WlSurface* wl_surface)
{
    scene::SurfaceCreationParameters params;

    {
        std::lock_guard<std::mutex> lock{mutex};

        if (creating_scene_surface || weak_scene_surface.lock())
            return;

        creating_scene_surface = true;

        params.type = mir_window_type_freestyle;
        if (!properties.title.empty())
            params.name = properties.title;
        if (!properties.appId.empty())
            params.application_id = properties.appId;
        params.size = wl_surface->buffer_size().value_or(init.size);
        params.server_side_decorated = true;
    }

    params.streams = std::vector<shell::StreamSpecification>{};
    params.input_shape = std::vector<geom::Rectangle>{};
    wl_surface->populate_surface_data(params.streams.value(), params.input_shape.value(), {});

    auto const observer = std::make_shared<XWaylandSurfaceObserver>(seat, wl_surface, this);
    auto const surface = shell->create_surface(get_session(wl_surface->resource), params, observer);

    {
        std::lock_guard<std::mutex> lock{mutex};
        surface_observer = observer;
        weak_scene_surface = surface;
        creating_scene_surface = false;
    }
}

void mf::XWaylandSurface::set_window_state(WindowState const& new_window_state)
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
        connection,
        XCB_PROP_MODE_REPLACE,
        window, xwm->xcb_atom.wm_state,
        xwm->xcb_atom.wm_state, 32,
        length_of(wm_state_properties), wm_state_properties);

    std::vector<xcb_atom_t> net_wm_states;

    if (new_window_state.minimized)
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
    // TODO: Set _NET_WM_STATE_MODAL if appropriate

    xcb_change_property(
        connection,
        XCB_PROP_MODE_REPLACE,
        window,
        xwm->xcb_atom.net_wm_state,
        XCB_ATOM_ATOM, 32, // type and format
        net_wm_states.size(), net_wm_states.data());

    MirWindowState mir_window_state;

    if (new_window_state.minimized)
        mir_window_state = mir_window_state_minimized;
    else if (new_window_state.fullscreen)
        mir_window_state = mir_window_state_fullscreen;
    else if (new_window_state.maximized)
        mir_window_state = mir_window_state_maximized;
    else
        mir_window_state = mir_window_state_restored;

    bool update_mir_window_state = false;

    {
        std::lock_guard<std::mutex> lock{mutex};

        window_state = new_window_state;

        if (mir_window_state != cached_mir_window_state)
        {
            update_mir_window_state = true;
            cached_mir_window_state = mir_window_state;
        }
    }

    if (update_mir_window_state)
    {
        if (auto const scene_surface = weak_scene_surface.lock())
        {
            shell::SurfaceSpecification mods;
            mods.state = mir_window_state;
            shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
        }
    }
}

auto mf::XWaylandSurface::latest_input_timestamp(std::lock_guard<std::mutex> const&) -> std::chrono::nanoseconds
{
    if (surface_observer)
    {
        return surface_observer.value()->latest_timestamp();
    }
    else
    {
        log_warning("Can not get timestamp because surface_observer is null");
        return {};
    }
}
