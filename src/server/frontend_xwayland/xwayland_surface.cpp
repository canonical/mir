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

#include "xwayland_surface.h"
#include "xwayland_log.h"
#include "xwayland_surface_observer.h"
#include "xwayland_client_manager.h"
#include "xwayland_wm_shell.h"
#include "xwayland_surface_role.h"

#include "mir/frontend/wayland.h"
#include "mir/scene/surface.h"
#include "mir/shell/decoration.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/events/input_event.h"

#include "boost/throw_exception.hpp"

#include <algorithm>
#include <sstream>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mw = mir::wayland;
namespace geom = mir::geometry;

namespace
{
/// See ICCCM 4.1.3.1 (https://tronche.com/gui/x/icccm/sec-4.html#WM_HINTS)
namespace WmHintsIndices
{
enum WmHintsIndices: uint32_t
{
    FLAGS = 0,
    INPUT,
    INITIAL_STATE,
    ICON_PIXMAP,
    ICON_WINDOW,
    ICON_X,
    ICON_Y,
    ICON_MASK,
};
}

/// See ICCCM 4.1.3.1 (https://tronche.com/gui/x/icccm/sec-4.html#WM_HINTS)
namespace WmHintsFlags
{
enum WmHintsFlags: uint32_t
{
    INPUT = 1,
    STATE = 2,
    ICON_PIXMAP = 4,
    ICON_WINDOW = 8,
    ICON_POSITION = 16,
    ICON_MASK = 32,
    WINDOW_GROUP = 64,
    MESSAGE = 128,
    URGENCY = 256,
};
}

// See ICCCM 4.1.2.3 (https://tronche.com/gui/x/icccm/sec-4.html#s-4.1.2.3)
// except actually I'm pretty sure that mistakenly drops min size/aspect so actually see anything that implements it
// such as https://stackoverflow.com/a/59762666
namespace WmSizeHintsIndices
{
enum WmSizeHintsIndices: unsigned
{
    FLAGS = 0,
    X, Y,
    WIDTH, HEIGHT,
    MIN_WIDTH, MIN_HEIGHT,
    MAX_WIDTH, MAX_HEIGHT,
    WIDTH_INC, HEIGHT_INC,
    MIN_ASPECT_NUM, MIN_ASPECT_DEN,
    MAX_ASPECT_NUM, MAX_ASPECT_DEN,
    BASE_WIDTH, BASE_HEIGHT,
    WIN_GRAVITY,
    END,
};
}

/// See ICCCM 4.1.2.3 (https://tronche.com/gui/x/icccm/sec-4.html#s-4.1.2.3)
namespace WmSizeHintsFlags
{
enum WmSizeHintsFlags: uint32_t
{
    POSITION_FROM_USER = 1, // User-specified x, y
    SIZE_FROM_USER = 2, // User-specified width, height
    POSITION_FROM_CLEINT = 4, // Program-specified position
    SIZE_FROM_CLIENT = 8, // Program-specified size
    MIN_SIZE = 16, // Program-specified minimum size
    MAX_SIZE = 32, // Program-specified maximum size
    RESIZE_INC = 64, // Program-specified resize increments
    ASPECT = 128, // Program-specified min and max aspect ratios
    BASE_SIZE = 256, // Program-specified base size
    GRAVITY = 512, // Program-specified window gravity
};
}

/// See https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#sourceindication
enum class SourceIndication: uint32_t
{
    UNKNOWN = 0,
    APPLICATION = 1,
    PAGER = 2,
};

///See https://specifications.freedesktop.org/wm-spec/latest/ar01s04.html
enum class NetWmMoveresize: uint32_t
{
    SIZE_TOPLEFT = 0,
    SIZE_TOP = 1,
    SIZE_TOPRIGHT = 2,
    SIZE_RIGHT = 3,
    SIZE_BOTTOMRIGHT = 4,
    SIZE_BOTTOM = 5,
    SIZE_BOTTOMLEFT = 6,
    SIZE_LEFT = 7,
    MOVE = 8,           /* movement only */
    SIZE_KEYBOARD = 9,  /* size via keyboard */
    MOVE_KEYBOARD = 10, /* move via keyboard */
    CANCEL = 11,        /* cancel operation */
};

// Any standard for the motif hints seems to be lost to time, but Weston has a reasonable definition:
// https://github.com/wayland-project/weston/blob/f7f8f5f1a87dd697ad6de74a885493bcca920cde/xwayland/window-manager.c#L78
namespace MotifWmHintsIndices
{
enum MotifWmHintsIndices: unsigned
{
    FLAGS,
    FUNCTIONS,
    DECORATIONS,
    INPUT_MODE,
    STATUS,
    END,
};
}

namespace MotifWmHintsFlags
{
enum MotifWmHintsFlags: uint32_t
{
    FUNCTIONS = (1L << 0),
    DECORATIONS = (1L << 1),
    INPUT_MODE = (1L << 2),
    STATUS = (1L << 3),
};
}

auto wm_resize_edge_to_mir_resize_edge(NetWmMoveresize wm_resize_edge) -> std::optional<MirResizeEdge>
{
    switch (wm_resize_edge)
    {
    case NetWmMoveresize::SIZE_TOPLEFT:         return mir_resize_edge_northwest;
    case NetWmMoveresize::SIZE_TOP:             return mir_resize_edge_north;
    case NetWmMoveresize::SIZE_TOPRIGHT:        return mir_resize_edge_northeast;
    case NetWmMoveresize::SIZE_RIGHT:           return mir_resize_edge_east;
    case NetWmMoveresize::SIZE_BOTTOMRIGHT:     return mir_resize_edge_southeast;
    case NetWmMoveresize::SIZE_BOTTOM:          return mir_resize_edge_south;
    case NetWmMoveresize::SIZE_BOTTOMLEFT:      return mir_resize_edge_southwest;
    case NetWmMoveresize::SIZE_LEFT:            return mir_resize_edge_west;
    case NetWmMoveresize::MOVE:                 break;
    case NetWmMoveresize::SIZE_KEYBOARD:        break;
    case NetWmMoveresize::MOVE_KEYBOARD:        break;
    case NetWmMoveresize::CANCEL:               break;
    }

    return std::nullopt;
}

auto wm_window_type_to_mir_window_type(
    mf::XCBConnection* connection,
    std::vector<xcb_atom_t> const& wm_types,
    bool is_transient_for,
    bool override_redirect) -> MirWindowType
{
    if (mir::verbose_xwayland_logging_enabled())
    {
        std::stringstream message;

        for (auto const& wm_type : wm_types)
        {
            if (wm_type == connection->_NET_WM_WINDOW_TYPE_NORMAL)
                message << "\n\t_NET_WM_WINDOW_TYPE_NORMAL";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_SPLASH)
                message << "\n\t_NET_WM_WINDOW_TYPE_SPLASH";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_DESKTOP)
                message << "\n\t_NET_WM_WINDOW_TYPE_DESKTOP";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_TOOLBAR)
                message << "\n\t_NET_WM_WINDOW_TYPE_TOOLBAR";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_MENU)
                message << "\n\t_NET_WM_WINDOW_TYPE_MENU";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_UTILITY)
                message << "\n\t_NET_WM_WINDOW_TYPE_UTILITY";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_POPUP_MENU)
                message << "\n\t_NET_WM_WINDOW_TYPE_POPUP_MENU";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
                message << "\n\t_NET_WM_WINDOW_TYPE_DROPDOWN_MENU";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_COMBO)
                message << "\n\t_NET_WM_WINDOW_TYPE_COMBO";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_TOOLTIP)
                message << "\n\t_NET_WM_WINDOW_TYPE_TOOLTIP";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_NOTIFICATION)
                message << "\n\t_NET_WM_WINDOW_TYPE_NOTIFICATION";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_DND)
                message << "\n\t_NET_WM_WINDOW_TYPE_DND";
            else if (wm_type == connection->_NET_WM_WINDOW_TYPE_DIALOG)
                message << "\n\t_NET_WM_WINDOW_TYPE_DIALOG";
        }

        if (is_transient_for)
            message << "\n\tis_transient_for";
        if (override_redirect)
            message << "\n\toverride_redirect";

        mir::log_debug("%s%s\n~~~~~~~~~~~~~~", __PRETTY_FUNCTION__ , message.str().c_str());
    }

    // See https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html#idm46515148839648
    for (auto const& wm_type : wm_types)
    {
        if (wm_type == connection->_NET_WM_WINDOW_TYPE_NORMAL ||
            wm_type == connection->_NET_WM_WINDOW_TYPE_SPLASH ||
            wm_type == connection->_NET_WM_WINDOW_TYPE_DESKTOP ||
            wm_type == connection->_NET_WM_WINDOW_TYPE_DOCK)
        {
            // TODO: perhaps desktop and dock should set the layer as well somehow?
            return mir_window_type_freestyle;
        }
        else if (wm_type == connection->_NET_WM_WINDOW_TYPE_TOOLBAR ||
                 wm_type == connection->_NET_WM_WINDOW_TYPE_MENU ||
                 wm_type == connection->_NET_WM_WINDOW_TYPE_UTILITY)
        {
            return mir_window_type_satellite;
        }
        else if (wm_type == connection->_NET_WM_WINDOW_TYPE_POPUP_MENU ||
                 wm_type == connection->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU ||
                 wm_type == connection->_NET_WM_WINDOW_TYPE_COMBO)
        {
            return mir_window_type_menu;
        }
        else if (wm_type == connection->_NET_WM_WINDOW_TYPE_DIALOG)
        {
            return override_redirect ? mir_window_type_menu : mir_window_type_dialog;
        }
        else if (wm_type == connection->_NET_WM_WINDOW_TYPE_TOOLTIP ||
                 wm_type == connection->_NET_WM_WINDOW_TYPE_NOTIFICATION ||
                 wm_type == connection->_NET_WM_WINDOW_TYPE_DND)
        {
            return mir_window_type_tip;
        }
        else if (mir::verbose_xwayland_logging_enabled())
        {
            mir::log_debug(
                "Ignoring unknown window type %s",
                connection->query_name(wm_type).c_str());
        }
    }

    // "_NET_WM_WINDOW_TYPE_DIALOG... If _NET_WM_WINDOW_TYPE is not set, then managed windows with WM_TRANSIENT_FOR set
    // MUST be taken as this type. Override-redirect windows with WM_TRANSIENT_FOR, but without _NET_WM_WINDOW_TYPE must
    // be taken as _NET_WM_WINDOW_TYPE_NORMAL."
    if (is_transient_for && !override_redirect)
    {
        return mir_window_type_menu;
    }
    else
    {
        return mir_window_type_freestyle;
    }
}

template<typename T>
auto property_handler(
    std::shared_ptr<mf::XCBConnection> const& connection,
    xcb_window_t window,
    xcb_atom_t property,
    mf::XCBConnection::Handler<T>&& handler) -> std::pair<xcb_atom_t, std::function<std::function<void()>()>>
{
    return std::make_pair(
        property,
        [connection, window, property, handler = std::move(handler)]()
        {
            return connection->read_property(window, property, std::move(handler));
        });
}

template<typename T>
auto property_handler(
    std::shared_ptr<mf::XCBConnection> const& connection,
    xcb_window_t window,
    xcb_atom_t property,
    std::function<void(T const&)> handler) -> std::pair<xcb_atom_t, std::function<std::function<void()>()>>
{
    return property_handler<T>(connection, window, property, mf::XCBConnection::Handler<T>{std::move(handler)});
}

template<typename T>
auto make_optional_if(bool condition, T&& value) -> std::optional<T>
{
    if (condition)
    {
        return std::optional<T>(std::move(value));
    }
    else
    {
        return std::nullopt;
    }
}
}

mf::XWaylandSurface::XWaylandSurface(
    XWaylandWM *wm,
    std::shared_ptr<XCBConnection> const& connection,
    XWaylandWMShell const& wm_shell,
    std::shared_ptr<XWaylandClientManager> const& client_manager,
    xcb_window_t window,
    geometry::Rectangle const& geometry,
    bool override_redirect,
    float scale)
    : xwm(wm),
      connection{connection},
      wm_shell{wm_shell},
      shell{wm_shell.shell},
      client_manager{client_manager},
      window(window),
      scale{scale},
      property_handlers{
          property_handler<std::string>(
              connection,
              window,
              XCB_ATOM_WM_CLASS,
              [this](auto value)
              {
                  std::lock_guard lock{mutex};
                  this->pending_spec(lock).application_id = value;
              }),
          property_handler<std::string>(
              connection,
              window,
              XCB_ATOM_WM_NAME,
              [this](auto value)
              {
                  std::lock_guard lock{mutex};
                  this->pending_spec(lock).name = value;
              }),
          property_handler<std::string>(
              connection,
              window,
              connection->_NET_WM_NAME,
              [this](auto value)
              {
                  std::lock_guard lock{mutex};
                  this->pending_spec(lock).name = value;
              }),
          property_handler<xcb_window_t>(
              connection,
              window,
              XCB_ATOM_WM_TRANSIENT_FOR,
              {
                  [this](xcb_window_t const& value)
                  {
                      is_transient_for(value);
                  },
                  [this](auto)
                  {
                      is_transient_for(XCB_WINDOW_NONE);
                  }
              }),
          property_handler<std::vector<xcb_atom_t>>(
              connection,
              window,
              connection->_NET_WM_WINDOW_TYPE,
              {
                  [this](auto wm_types)
                  {
                      has_window_types(wm_types);
                  },
                  [this](auto)
                  {
                      has_window_types({});
                  }
              }),
          property_handler<std::vector<int32_t>>(
              connection,
              window,
              connection->WM_HINTS,
              [this](auto hints)
              {
                  wm_hints(hints);
              }),
          property_handler<std::vector<int32_t>>(
              connection,
              window,
              connection->WM_NORMAL_HINTS,
              [this](auto hints)
              {
                  wm_size_hints(hints);
              }),
          property_handler<std::vector<xcb_atom_t>>(
              connection,
              window,
              connection->WM_PROTOCOLS,
              {
                  [this](auto value)
                  {
                      std::lock_guard lock{mutex};
                      this->cached.supported_wm_protocols = std::set<xcb_atom_t>{value.begin(), value.end()};
                  },
                  [this](auto)
                  {
                      std::lock_guard lock{mutex};
                      this->cached.supported_wm_protocols.clear();
                  }
              }),
          property_handler<std::vector<uint32_t>>(
              connection,
              window,
              connection->_MOTIF_WM_HINTS,
              [this](auto hints)
              {
                  motif_wm_hints(hints);
              })}
{
    cached.geometry = geometry;
    cached.override_redirect = override_redirect;

    uint32_t const value = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE;
    xcb_change_window_attributes(*connection, window, XCB_CW_EVENT_MASK, &value);
}

mf::XWaylandSurface::~XWaylandSurface()
{
    close();
}

void mf::XWaylandSurface::map()
{
    std::unique_lock lock{mutex};
    auto state = cached.state.with_withdrawn(false);
    lock.unlock();

    // _NET_WM_STATE is not in property_handlers because we only read it on window creation
    // We, the server (not the client) are responsible for updating it after the window has been mapped
    // The client should use a client message to change state later
    auto const cookie = connection->read_property(
        window,
        connection->_NET_WM_STATE,
        {
            [&](std::vector<xcb_atom_t> const& net_wm_states)
            {
                for (auto const& net_wm_state : net_wm_states)
                {
                    state = state.with_net_wm_state_change(*connection, NetWmStateAction::ADD, net_wm_state);
                }
            }
        });

    // If we had more properties to read we would queue them all up before completing the first one
    uint32_t const workspace = 1;
    connection->set_property<XCBType::CARDINAL32>(
        window,
        connection->_NET_WM_DESKTOP,
        workspace);

    inform_client_of_window_state(std::unique_lock(mutex), state);
    request_scene_surface_state(state.active_mir_state());
    xcb_map_window(*connection, window);
    connection->flush();
}

void mf::XWaylandSurface::close()
{
    std::shared_ptr<XWaylandClientManager::Session> local_client_session;
    std::shared_ptr<scene::Surface> scene_surface;
    XWaylandSurfaceObserverManager local_surface_observer_manager;

    {
        std::lock_guard lock{mutex};

        local_client_session = std::move(client_session);
        local_surface_observer_manager = std::move(surface_observer_manager);
        scene_surface = weak_scene_surface.lock();
        weak_scene_surface.reset();
    }

    if (scene_surface)
    {
        xwm->forget_scene_surface(scene_surface);
    }

    connection->delete_property(window, connection->_NET_WM_DESKTOP);

    inform_client_of_window_state(std::unique_lock(mutex), StateTracker::make_withdrawn());

    xcb_unmap_window(*connection, window);
    connection->flush();

    if (scene_surface)
    {
        shell->destroy_surface(scene_surface->session().lock(), scene_surface);
        scene_surface.reset();
        // Someone may still be holding on to the surface somewhere, and that's fine
    }

    local_client_session.reset();
}

void mf::XWaylandSurface::take_focus()
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("%s taking focus", connection->window_debug_string(window).c_str());
    }

    bool supports_take_focus;
    {
        std::lock_guard lock{mutex};

        if (cached.override_redirect)
            return;

        supports_take_focus = (
            cached.supported_wm_protocols.find(connection->WM_TAKE_FOCUS) !=
            cached.supported_wm_protocols.end());
    }

    if (supports_take_focus)
    {
        uint32_t const client_message_data[]{
            connection->WM_TAKE_FOCUS,
            XCB_TIME_CURRENT_TIME};
        uint32_t const event_mask = cached.input_hint ? XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT : XCB_EVENT_MASK_NO_EVENT;
        connection->send_client_message<XCBType::WM_PROTOCOLS>(window, event_mask, client_message_data);
    }

    // See https://tronche.com/gui/x/icccm/sec-4.html#s-4.1.7
    if (cached.input_hint)
    {
        xcb_set_input_focus(
            *connection,
            XCB_INPUT_FOCUS_POINTER_ROOT,
            window,
            XCB_CURRENT_TIME);
    }

    connection->flush();
}

auto correct_requested_size(geom::Size requested_size, std::weak_ptr<mir::scene::Surface> weak_surface) -> geom::Size
{
    if (auto surface = weak_surface.lock())
    {
        return mir::shell::decoration::compute_size_with_decorations(requested_size, surface->type(), surface->state());
    }

    return requested_size;
}

void mf::XWaylandSurface::configure_request(xcb_configure_request_event_t* event)
{
    std::unique_lock lock{mutex};
    if (event->value_mask & XCB_CONFIG_WINDOW_X || event->value_mask & XCB_CONFIG_WINDOW_Y)
    {
        pending_spec(lock).top_left = geom::Point{
            event->value_mask & XCB_CONFIG_WINDOW_X ? geom::X{event->x} : cached.geometry.left(),
            event->value_mask & XCB_CONFIG_WINDOW_Y ? geom::Y{event->y} : cached.geometry.top()};
    }

    // Mir seems to not like it when only one dimension is specified
    if (event->value_mask & XCB_CONFIG_WINDOW_WIDTH || event->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
    {
        auto const requested_width =
            event->value_mask & XCB_CONFIG_WINDOW_WIDTH ? geom::Width{event->width} : cached.geometry.size.width;
        auto const requested_height =
            event->value_mask & XCB_CONFIG_WINDOW_HEIGHT ? geom::Height{event->height} : cached.geometry.size.height;
        auto const corrected_size =
            correct_requested_size(geom::Size{requested_width, requested_height}, weak_scene_surface);
        pending_spec(lock).width = corrected_size.width;
        pending_spec(lock).height = corrected_size.height;
    }

    if (!weak_scene_surface.lock())
    {
        lock.unlock();

        inform_client_of_geometry(
            make_optional_if(event->value_mask & XCB_CONFIG_WINDOW_X, geom::X{event->x}),
            make_optional_if(event->value_mask & XCB_CONFIG_WINDOW_Y, geom::Y{event->y}),
            make_optional_if(event->value_mask & XCB_CONFIG_WINDOW_WIDTH, geom::Width{event->width}),
            make_optional_if(event->value_mask & XCB_CONFIG_WINDOW_HEIGHT, geom::Height{event->height}));

        connection->flush();
    }
    else
    {
        lock.unlock();
        apply_any_mods_to_scene_surface();
    }

}

void mf::XWaylandSurface::configure_notify(xcb_configure_notify_event_t* event)
{
    std::unique_lock lock{mutex};
    cached.override_redirect = event->override_redirect;
    // If this configure is in response to a configure we sent we don't want to make a window manager request
    auto const geometry = geom::Rectangle{geom::Point{event->x, event->y}, geom::Size{event->width, event->height}};
    auto const it = std::find(inflight_configures.begin(), inflight_configures.end(), geometry);
    if (it == inflight_configures.end())
    {
        if (verbose_xwayland_logging_enabled())
        {
            log_debug("Processing configure notify because it came from someone else");
        }

        auto const corrected_size = correct_requested_size(geometry.size, weak_scene_surface);

        cached.geometry = geometry;
        pending_spec(lock).top_left = geometry.top_left;
        pending_spec(lock).width = corrected_size.width;
        pending_spec(lock).height = corrected_size.height;
        lock.unlock();
        apply_any_mods_to_scene_surface();
    }
    else
    {
        if (verbose_xwayland_logging_enabled())
        {
            log_debug("Ignoring configure notify because it came from us");
        }
        inflight_configures.erase(inflight_configures.begin(), it + 1);
    }
}

void mf::XWaylandSurface::net_wm_state_client_message(uint32_t const (&data)[5])
{
    // The client is requesting a change in state
    // See https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45805407959456

    auto const* pdata = data;
    auto const action = static_cast<NetWmStateAction>(*pdata++);
    xcb_atom_t const properties[2] = { static_cast<xcb_atom_t>(*pdata++),  static_cast<xcb_atom_t>(*pdata++) };
    auto const source_indication = static_cast<SourceIndication>(*pdata++);

    (void)source_indication;

    std::unique_lock lock{mutex};
    auto new_state = cached.state;
    for (xcb_atom_t const property : properties)
    {
        if (property) // if there is only one property, the 2nd is 0
        {
            new_state = new_state.with_net_wm_state_change(*connection, action, property);
        }
    }
    inform_client_of_window_state(std::move(lock), new_state);

    request_scene_surface_state(new_state.active_mir_state());
}

void mf::XWaylandSurface::wm_change_state_client_message(uint32_t const (&data)[5])
{
    // See ICCCM 4.1.4 (https://tronche.com/gui/x/icccm/sec-4.html)

    WmState const requested_state = static_cast<WmState>(data[0]);

    std::unique_lock lock{mutex};
    auto const new_state = cached.state.with_wm_state(requested_state);
    inform_client_of_window_state(std::move(lock), new_state);

    request_scene_surface_state(new_state.active_mir_state());
}

void mf::XWaylandSurface::property_notify(xcb_atom_t property)
{
    auto const handler = property_handlers.find(property);
    if (handler != property_handlers.end())
    {
        auto completion = handler->second();
        completion();

        apply_any_mods_to_scene_surface();
    }
}

void mf::XWaylandSurface::attach_wl_surface(WlSurface* wl_surface)
{
    // We assume we are on the Wayland thread

    if (verbose_xwayland_logging_enabled())
    {
        log_debug(
            "Attaching wl_surface@%d to %s...",
            wl_resource_get_id(wl_surface->resource),
            connection->window_debug_string(window).c_str());
    }

    auto state = StateTracker::make_withdrawn();
    shell::SurfaceSpecification spec;
    std::vector<std::shared_ptr<void>> keep_alive_until_spec_is_used;

    auto const observer = std::make_shared<XWaylandSurfaceObserver>(
        *wm_shell.wayland_executor,
        wm_shell.seat,
        wl_surface,
        this,
        scale);

    {
        std::lock_guard lock{mutex};

        state = cached.state;

        XWaylandSurfaceRole::populate_surface_data_scaled(wl_surface, scale, spec, keep_alive_until_spec_is_used);

        auto const window_type = mir_window_type_freestyle;
        auto const window_state = state.active_mir_state();
        auto const corrected_size = mir::shell::decoration::compute_size_with_decorations(
            cached.geometry.size, window_type, window_state);

        // May be overridden by anything in the pending spec
        spec.width = corrected_size.width;
        spec.height = corrected_size.height;
        spec.top_left = cached.geometry.top_left;
        spec.type = window_type;
        spec.state = window_state;
    }


    std::vector<std::function<void()>> reply_functions;

    // Read all properties
    for (auto const& handler : property_handlers)
    {
        reply_functions.push_back(handler.second());
    }

    std::shared_ptr<XWaylandClientManager::Session> local_client_session;
    std::shared_ptr<ms::Session> session;
    bool rejected = false;
    reply_functions.push_back(connection->read_property(
        window, connection->_NET_WM_PID,
        XCBConnection::Handler<uint32_t>{
            [&](uint32_t pid)
            {
                local_client_session = client_manager->session_for_client(pid);
                if (!local_client_session)
                {
                    rejected = true;
                    return;
                }
                session = local_client_session->session();
            },
            [&](std::string const&)
            {
                log_warning("X11 app did not set _NET_WM_PID, grouping it under the default XWayland application");
                session = get_session(wl_surface->resource);
            }
        }));

    // Wait for and process all the XCB replies
    for (auto const& reply_function : reply_functions)
    {
        reply_function();
    }

    if (rejected)
    {
        xcb_kill_client(*connection, window);
        close();
        connection->flush();
        return;
    }

    if (!session)
    {
        fatal_error("Property handlers did not set a valid session");
    }

    // property_handlers will have updated the pending spec. Use it.
    {
        std::lock_guard lock{mutex};

        if (auto const pending_spec = consume_pending_spec(lock))
        {
            spec.update_from(*pending_spec.value());
        }
        spec.server_side_decorated = !cached.override_redirect && !cached.motif_decorations_disabled;
        prep_surface_spec(lock, spec);
    }

    auto const surface = shell->create_surface(session, mw::make_weak(wl_surface), spec, observer, nullptr);
    XWaylandSurfaceObserverManager local_surface_observer_manager{surface, std::move(observer)};
    inform_client_of_window_state(std::unique_lock{mutex}, state);
    auto const top_left = scaled_top_left_of(*surface) + scaled_content_offset_of(*surface);
    auto const size = scaled_content_size_of(*surface);
    inform_client_of_geometry(top_left.x, top_left.y, size.width, size.height);
    connection->configure_window(
        window,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        XCB_STACK_MODE_ABOVE);

    {
        std::lock_guard lock{mutex};
        client_session = local_client_session;
        weak_scene_surface = surface;
        surface_observer_manager = std::move(local_surface_observer_manager);
    }

    xwm->remember_scene_surface(surface, window);

    // We might have had property changes between updating the params and setting
    // weak_scene_surface. Without weak_scene_surface they won't have been applied.
    // Don't drop them on the floor.
    apply_any_mods_to_scene_surface();
}

void mf::XWaylandSurface::move_resize(uint32_t detail)
{
    std::shared_ptr<scene::Surface> scene_surface;
    std::shared_ptr<MirInputEvent const> event;
    {
        std::lock_guard lock{mutex};
        scene_surface = weak_scene_surface.lock();
        event = surface_observer_manager.try_get_resize_event();

        if (!scene_surface || !event)
        {
            return;
        }
    }

    auto const action = static_cast<NetWmMoveresize>(detail);
    if (action == NetWmMoveresize::MOVE)
    {
        shell->request_move(scene_surface->session().lock(), scene_surface, event.get());
    }
    else if (auto const edge = wm_resize_edge_to_mir_resize_edge(action))
    {
        shell->request_resize(scene_surface->session().lock(), scene_surface, event.get(), edge.value());
    }
    else
    {
        mir::log_warning("XWaylandSurface::move_resize() called with unknown detail %d", detail);
    }
}

auto mf::XWaylandSurface::StateTracker::wm_state() const -> WmState
{
    if (withdrawn)
    {
        return WmState::WITHDRAWN;
    }
    else if (mir_state.has_any({mir_window_state_hidden, mir_window_state_minimized}))
    {
        return WmState::ICONIC;
    }
    else
    {
        return WmState::NORMAL;
    }
}

auto mf::XWaylandSurface::StateTracker::with_wm_state(WmState wm_state) const -> StateTracker
{
    switch (wm_state)
    {
    case WmState::NORMAL:
        return StateTracker{
            mir_state.without(mir_window_state_minimized).without(mir_window_state_hidden),
            withdrawn};

    case WmState::ICONIC:
        return StateTracker{mir_state.with(mir_window_state_minimized), withdrawn};

    default:
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Invalid WM_STATE: " + std::to_string(static_cast<std::underlying_type<WmState>::type>(wm_state))));
    }
}

auto mf::XWaylandSurface::StateTracker::net_wm_state(
    XCBConnection const& conn) const -> std::optional<std::vector<xcb_atom_t>>
{
    if (withdrawn)
    {
        return std::nullopt;
    }
    else
    {
        std::vector<xcb_atom_t> net_wm_states;
        if (mir_state.has_any({mir_window_state_hidden, mir_window_state_minimized}))
        {
            net_wm_states.push_back(conn._NET_WM_STATE_HIDDEN);
        }
        if (mir_state.has(mir_window_state_horizmaximized))
        {
            net_wm_states.push_back(conn._NET_WM_STATE_MAXIMIZED_HORZ);
        }
        if (mir_state.has(mir_window_state_vertmaximized))
        {
            net_wm_states.push_back(conn._NET_WM_STATE_MAXIMIZED_VERT);
        }
        if (mir_state.has(mir_window_state_fullscreen))
        {
            net_wm_states.push_back(conn._NET_WM_STATE_FULLSCREEN);
        }
        // TODO: Set _NET_WM_STATE_MODAL if appropriate and add it to supported atoms in XWaylandWM constructor
        return net_wm_states;
    }
}

auto mf::XWaylandSurface::StateTracker::with_net_wm_state_change(
    XCBConnection const& conn,
    NetWmStateAction action,
    xcb_atom_t net_wm_state) const -> StateTracker
{
    MirWindowState state;
    if (net_wm_state == conn._NET_WM_STATE_HIDDEN)
    {
        state = mir_window_state_minimized;
    }
    else if (net_wm_state == conn._NET_WM_STATE_MAXIMIZED_HORZ)
    {
        state = mir_window_state_horizmaximized;
    }
    else if (net_wm_state == conn._NET_WM_STATE_MAXIMIZED_VERT)
    {
        state = mir_window_state_vertmaximized;
    }
    else if (net_wm_state == conn._NET_WM_STATE_FULLSCREEN)
    {
        state = mir_window_state_fullscreen;
    }
    else
    {
        if (mir::verbose_xwayland_logging_enabled())
        {
            mir::log_debug(
                "Ignoring unknown _NET_WM_STATE %s",
                conn.query_name(net_wm_state).c_str());
        }
        return *this;
    }

    if (action == NetWmStateAction::TOGGLE)
    {
        action = mir_state.has(state) ? NetWmStateAction::REMOVE : NetWmStateAction::ADD;
    }

    switch (action)
    {
    case NetWmStateAction::ADD:     return StateTracker{mir_state.with(state), withdrawn};
    case NetWmStateAction::REMOVE:  return StateTracker{mir_state.without(state), withdrawn};
    default: return *this;
    }
}

void mf::XWaylandSurface::scene_surface_focus_set(bool has_focus)
{
    xwm->set_focus(window, has_focus);
}

void mf::XWaylandSurface::scene_surface_state_set(MirWindowState new_state)
{
    std::unique_lock lock{mutex};
    auto const state = cached.state.with_active_mir_state(new_state);
    inform_client_of_window_state(std::move(lock), state);

    if (new_state == mir_window_state_minimized || new_state == mir_window_state_hidden)
    {
        connection->configure_window(
            window,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            XCB_STACK_MODE_BELOW);
    }
}

void mf::XWaylandSurface::scene_surface_resized(geometry::Size const& new_size)
{
    inform_client_of_geometry(std::nullopt, std::nullopt, new_size.width, new_size.height);
    connection->flush();
}

void mf::XWaylandSurface::scene_surface_moved_to(geometry::Point const& new_top_left)
{
    std::unique_lock lock{mutex};
    auto const scene_surface = weak_scene_surface.lock();
    lock.unlock();

    auto const content_offset = scene_surface ? scaled_content_offset_of(*scene_surface) : geom::Displacement{};
    auto const offset_new_top_left = new_top_left + content_offset;
    inform_client_of_geometry(offset_new_top_left.x, offset_new_top_left.y, std::nullopt, std::nullopt);
    connection->flush();
}

void mf::XWaylandSurface::scene_surface_close_requested()
{
    bool delete_window;
    {
        std::lock_guard lock{mutex};
        delete_window = (
            cached.supported_wm_protocols.find(connection->WM_DELETE_WINDOW) !=
            cached.supported_wm_protocols.end());
    }

    if (delete_window)
    {
        if (verbose_xwayland_logging_enabled())
        {
            log_debug(
                "Sending WM_DELETE_WINDOW request to %s",
                connection->window_debug_string(window).c_str());
        }
        uint32_t const client_message_data[]{
            connection->WM_DELETE_WINDOW,
            XCB_TIME_CURRENT_TIME,
        };
        connection->send_client_message<XCBType::WM_PROTOCOLS>(window, XCB_EVENT_MASK_NO_EVENT, client_message_data);
    }
    else
    {
        if (verbose_xwayland_logging_enabled())
        {
            log_debug(
                "Not closing %s (because it does not support WM_DELETE_WINDOW)",
                connection->window_debug_string(window).c_str());
        }
    }
    connection->flush();
}

void mf::XWaylandSurface::wl_surface_destroyed()
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("%s's wl_surface destoyed", connection->window_debug_string(window).c_str());
    }
    close();
}

auto mf::XWaylandSurface::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    std::lock_guard lock{mutex};
    if (auto const scene_surface = weak_scene_surface.lock())
        return scene_surface;
    else
        return std::nullopt;
}

auto mf::XWaylandSurface::pending_spec(ProofOfMutexLock const&) -> msh::SurfaceSpecification&
{
    if (!nullable_pending_spec)
        nullable_pending_spec = std::make_unique<msh::SurfaceSpecification>();
    return *nullable_pending_spec;
}

auto mf::XWaylandSurface::consume_pending_spec(
    ProofOfMutexLock const&) -> std::optional<std::unique_ptr<msh::SurfaceSpecification>>
{
    if (nullable_pending_spec)
        return std::move(nullable_pending_spec);
    else
        return std::nullopt;
}

void mf::XWaylandSurface::is_transient_for(xcb_window_t transient_for)
{
    if (verbose_xwayland_logging_enabled())
    {
        if (transient_for != XCB_WINDOW_NONE)
        {
            log_debug("%s set as transient for %s",
                      connection->window_debug_string(window).c_str(),
                      connection->window_debug_string(transient_for).c_str());
        }
        else
        {
            log_debug(
                "%s is not transient",
                connection->window_debug_string(window).c_str());
        }
    }

    std::lock_guard lock{mutex};
    cached.transient_for = transient_for;
    apply_cached_transient_for_and_type(lock);
}

void mf::XWaylandSurface::has_window_types(std::vector<xcb_atom_t> const& wm_types)
{
    std::lock_guard lock{mutex};
    cached.wm_types = wm_types;
    apply_cached_transient_for_and_type(lock);
}

void mf::XWaylandSurface::inform_client_of_window_state(std::unique_lock<std::mutex> lock, StateTracker new_state)
{
    if (!lock.owns_lock())
    {
        lock.lock();
    }
    auto const old_state = cached.state;
    cached.state = new_state;
    if (old_state == new_state)
    {
        return;
    }
    lock.unlock();

    WmState const wm_state = new_state.wm_state();
    if (wm_state != old_state.wm_state())
    {
        uint32_t const wm_state_properties[]{
            static_cast<uint32_t>(wm_state),
            XCB_WINDOW_NONE // Icon window
        };
        connection->set_property<XCBType::WM_STATE>(window, connection->WM_STATE, wm_state_properties);
    }

    if (auto const net_wm_states = new_state.net_wm_state(*connection))
    {
        connection->set_property<XCBType::ATOM>(window, connection->_NET_WM_STATE, net_wm_states.value());
    }
    else
    {
        xcb_delete_property(*connection, window, connection->_NET_WM_STATE);
    }

    connection->flush();
}

void mf::XWaylandSurface::inform_client_of_geometry(
    std::optional<geometry::X> x,
    std::optional<geometry::Y> y,
    std::optional<geometry::Width> width,
    std::optional<geometry::Height> height)
{
    std::unique_lock lock{mutex};
    auto const geometry = geom::Rectangle{
        {x.value_or(cached.geometry.left()), y.value_or(cached.geometry.top())},
        {width.value_or(cached.geometry.size.width), height.value_or(cached.geometry.size.height)}};
    if (geometry == cached.geometry)
    {
        return;
    }
    cached.geometry = geometry;
    inflight_configures.push_back(geometry);
    while (inflight_configures.size() > 1000)
    {
        inflight_configures.pop_front();
        log_warning("%s inflight_configures buffer capped", connection->window_debug_string(window).c_str());
    }
    lock.unlock();

    if (verbose_xwayland_logging_enabled())
    {
        log_debug("configuring %s:", connection->window_debug_string(window).c_str());
        log_debug("            position: %d, %d", geometry.left().as_int(), geometry.top().as_int());
        log_debug("            size: %dx%d", geometry.size.width.as_int(), geometry.size.height.as_int());
    }

    connection->configure_window(
        window,
        geometry.top_left,
        geometry.size,
        std::nullopt,
        std::nullopt);
}

void mf::XWaylandSurface::request_scene_surface_state(MirWindowState new_state)
{
    std::shared_ptr<scene::Surface> scene_surface;

    {
        std::lock_guard lock{mutex};
        scene_surface = weak_scene_surface.lock();
    }

    if (scene_surface && scene_surface->state() != new_state)
    {
        shell::SurfaceSpecification mods;
        mods.state = new_state;
        // Just state is set so no need for scale_surface_spec()
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
    }
}

void mf::XWaylandSurface::apply_any_mods_to_scene_surface()
{
    std::shared_ptr<mir::scene::Surface> scene_surface;
    std::optional<std::unique_ptr<mir::shell::SurfaceSpecification>> spec;

    {
        std::lock_guard lock{mutex};
        if ((scene_surface = weak_scene_surface.lock()))
        {
            spec = consume_pending_spec(lock);
            if (spec)
            {
                prep_surface_spec(lock, *spec.value());
            }
        }
    }

    if (spec && scene_surface)
    {
        if (spec.value()->application_id.is_set() &&
            spec.value()->application_id.value() == scene_surface->application_id())
            spec.value()->application_id.consume();

        if (spec.value()->name.is_set() &&
            spec.value()->name.value() == scene_surface->name())
            spec.value()->name.consume();

        if (spec.value()->parent.is_set() &&
            spec.value()->parent.value().lock() == scene_surface->parent())
            spec.value()->parent.consume();

        if (spec.value()->type.is_set() &&
            spec.value()->type.value() == scene_surface->type())
            spec.value()->type.consume();

        if (spec.value()->top_left.is_set() &&
            spec.value()->top_left.value() == scene_surface->top_left())
            spec.value()->top_left.consume();

        if (spec.value()->width.is_set() &&
            spec.value()->width.value() == scene_surface->content_size().width)
            spec.value()->width.consume();

        if (spec.value()->height.is_set() &&
            spec.value()->height.value() == scene_surface->content_size().height)
            spec.value()->height.consume();

        if (!spec.value()->is_empty())
        {
            shell->modify_surface(scene_surface->session().lock(), scene_surface, *spec.value());
        }
    }
}

void mf::XWaylandSurface::prep_surface_spec(ProofOfMutexLock const&, msh::SurfaceSpecification& mods)
{
    auto const inv_scale = 1.0f / scale;

    if (mods.top_left)
    {
        auto const surface = weak_scene_surface.lock();
        auto const content_offset = surface ? surface->content_offset() : geom::Displacement{};
        if (auto const parent = effective_parent.lock())
        {
            auto const parent_content_top_left = parent->top_left() + parent->content_offset();
            auto const local_top_left = as_point(
                as_displacement(mods.top_left.value()) * inv_scale - as_displacement(parent_content_top_left));
            mods.aux_rect = {local_top_left, {1, 1}};
            mods.placement_hints = MirPlacementHints{};
            mods.surface_placement_gravity = mir_placement_gravity_northwest;
            mods.aux_rect_placement_gravity = mir_placement_gravity_northwest;
            mods.top_left.consume();
        }
        else
        {
            mods.top_left = as_point(as_displacement(mods.top_left.value()) * inv_scale - content_offset);
        }
    }

    auto scale_size_clamped = [inv_scale](auto& optional_prop)
        {
            if (optional_prop)
            {
                using ValueType = typename std::remove_reference<decltype(optional_prop.value())>::type;
                using UnderlyingType = typename ValueType::ValueType;
                auto constexpr raw_max = std::numeric_limits<UnderlyingType>::max();

                double const double_value = optional_prop.value().as_value() * inv_scale;
                optional_prop = std::max(ValueType{1}, ValueType{std::min(double_value, double{raw_max})});
            }
        };

    scale_size_clamped(mods.width);
    scale_size_clamped(mods.height);
    scale_size_clamped(mods.min_width);
    scale_size_clamped(mods.min_height);
    scale_size_clamped(mods.max_width);
    scale_size_clamped(mods.max_height);

    // NOTE: exclusive rect not checked because it is not used by XWayland surfaces
    // NOTE: aux rect related properties not checks because they are only set in this method
    // NOTE: buffer streams and input shapes are set and thus fixed in XWaylandSurfaceRole
}

auto mf::XWaylandSurface::scaled_top_left_of(ms::Surface const& surface) -> geometry::Point
{
    return as_point(as_displacement(surface.top_left()) * scale);
}

auto mf::XWaylandSurface::scaled_content_offset_of(ms::Surface const& surface) -> geometry::Displacement
{
    return surface.content_offset() * scale;
}

auto mf::XWaylandSurface::scaled_content_size_of(ms::Surface const& surface) -> geometry::Size
{
    return surface.content_size() * scale;
}

auto mf::XWaylandSurface::plausible_parent(ProofOfMutexLock const&) -> std::shared_ptr<ms::Surface>
{
    if (auto const current_effective = effective_parent.lock())
    {
        return current_effective;
    }

    // Taking the focussed window is plausible, but it is just a best guess. Having focus means is the most likely one
    // to be interacting with the user.
    if (auto const focused_window = xwm->get_focused_window())
    {
        // We don't want to be our own parent, that would be weird
        if (focused_window != window)
        {
            if (auto const parent = xcb_window_get_scene_surface(xwm, focused_window.value()))
            {
                if (verbose_xwayland_logging_enabled())
                {
                    log_debug(
                        "Set parent of %s from xwm->get_focused_window() (%s)",
                        connection->window_debug_string(window).c_str(),
                        connection->window_debug_string(focused_window.value()).c_str());
                }
                return parent;
            }
        }
    }

    if (verbose_xwayland_logging_enabled())
    {
        log_debug("Unable to find suitable parent for %s", connection->window_debug_string(window).c_str());
    }
    return {};
}

void mf::XWaylandSurface::apply_cached_transient_for_and_type(ProofOfMutexLock const& lock)
{
    auto type = wm_window_type_to_mir_window_type(
        connection.get(),
        cached.wm_types,
        cached.transient_for,
        cached.override_redirect);
    auto parent = xcb_window_get_scene_surface(xwm, cached.transient_for);
    if (type == mir_window_type_dialog ||
        type == mir_window_type_menu ||
        type == mir_window_type_satellite ||
        type == mir_window_type_tip ||
        type == mir_window_type_gloss)
    {
        // Type should have parent
        if (!parent)
        {
            parent = plausible_parent(lock);
            if (!parent)
            {
                type = mir_window_type_freestyle;
            }
        }
    }

    effective_parent = parent;

    auto& spec = pending_spec(lock);
    spec.parent = parent;
    spec.type = type;
}

void mf::XWaylandSurface::wm_hints(std::vector<int32_t> const& hints)
{
    // See ICCCM 4.1.2.4 (https://tronche.com/gui/x/icccm/sec-4.html#WM_HINTS)
    std::lock_guard lock{mutex};
    auto const flags = static_cast<uint32_t>(hints[WmHintsIndices::FLAGS]);
    if (flags & WmHintsFlags::INPUT)
    {
        cached.input_hint = hints[WmHintsIndices::INPUT];
        if (verbose_xwayland_logging_enabled())
        {
            log_debug(
                "%s input hint set to %s",
                connection->window_debug_string(window).c_str(),
                hints[WmHintsIndices::INPUT] ? "true" : "false");
        }
    }
}

void mf::XWaylandSurface::wm_size_hints(std::vector<int32_t> const& hints)
{
    // See ICCCM 4.1.2.3 (https://tronche.com/gui/x/icccm/sec-4.html#s-4.1.2.3)
    // except actually I'm pretty sure that mistakenly drops min size so actually see anything that implements it
    std::lock_guard lock{mutex};
    if (hints.size() != WmSizeHintsIndices::END)
    {
        log_error("WM_NORMAL_HINTS only has %zu element(s)", hints.size());
        return;
    }
    auto const flags = static_cast<uint32_t>(hints[WmSizeHintsIndices::FLAGS]);
    if (flags & WmSizeHintsFlags::MIN_SIZE)
    {
        pending_spec(lock).min_width = geom::Width{hints[WmSizeHintsIndices::MIN_WIDTH]};
        pending_spec(lock).min_height = geom::Height{hints[WmSizeHintsIndices::MIN_HEIGHT]};
        if (verbose_xwayland_logging_enabled())
        {
            log_debug(
                "%s min size set to %dx%d",
                connection->window_debug_string(window).c_str(),
                hints[WmSizeHintsIndices::MIN_WIDTH],
                hints[WmSizeHintsIndices::MIN_HEIGHT]);
        }
    }
    if (flags & WmSizeHintsFlags::MAX_SIZE)
    {
        pending_spec(lock).max_width = geom::Width{hints[WmSizeHintsIndices::MAX_WIDTH]};
        pending_spec(lock).max_height = geom::Height{hints[WmSizeHintsIndices::MAX_HEIGHT]};
        if (verbose_xwayland_logging_enabled())
        {
            log_debug(
                "%s max size set to %dx%d",
                connection->window_debug_string(window).c_str(),
                hints[WmSizeHintsIndices::MAX_WIDTH],
                hints[WmSizeHintsIndices::MAX_HEIGHT]);
        }
    }
}

void mf::XWaylandSurface::motif_wm_hints(std::vector<uint32_t> const& hints)
{
    std::lock_guard lock{mutex};
    if (hints.size() != MotifWmHintsIndices::END)
    {
        log_error("_MOTIF_WM_HINTS value has incorrect size %zu", hints.size());
        return;
    }
    if (MotifWmHintsFlags::DECORATIONS & hints[MotifWmHintsIndices::FLAGS])
    {
        // Disable decorations only if all flags are off
        cached.motif_decorations_disabled = (hints[MotifWmHintsIndices::DECORATIONS] == 0);
    }
}

auto mf::XWaylandSurface::xcb_window_get_scene_surface(
    mf::XWaylandWM* xwm,
    xcb_window_t window) -> std::shared_ptr<ms::Surface>
{
    if (auto const xwayland_surface = xwm->get_wm_surface(window))
    {
        if (auto const scene_surface = xwayland_surface.value()->scene_surface())
        {
            return scene_surface.value();
        }
    }

    return {};
}

mf::XWaylandSurface::XWaylandSurfaceObserverManager::XWaylandSurfaceObserverManager() = default;

mf::XWaylandSurface::XWaylandSurfaceObserverManager::XWaylandSurfaceObserverManager(
    std::shared_ptr<scene::Surface> scene_surface,
    std::shared_ptr<XWaylandSurfaceObserver> surface_observer) :
    weak_scene_surface{scene_surface},
    surface_observer{std::move(surface_observer)}
{
}

mf::XWaylandSurface::XWaylandSurfaceObserverManager::~XWaylandSurfaceObserverManager()
{
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        scene_surface->unregister_interest(*surface_observer);
    }
}

auto mf::XWaylandSurface::XWaylandSurfaceObserverManager::try_get_resize_event()
-> std::shared_ptr<MirInputEvent const>
{
    if (surface_observer)
    {
        return surface_observer->latest_move_resize_event();
    }
    else
    {
        return std::shared_ptr<MirInputEvent const>{};
    }
}
