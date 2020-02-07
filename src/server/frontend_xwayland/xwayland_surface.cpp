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
#include <algorithm>
#include <experimental/optional>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
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

/// Sets up the position, either as a child window with and aux rect or a toplevel
/// Parent can be nullptr
/// top_left should be desired global top_left of the decorations of this window
void set_position(std::shared_ptr<ms::Surface> parent, geom::Point top_left, msh::SurfaceSpecification& spec)
{
    if (parent)
    {
        auto const local_top_left =
            top_left -
            as_displacement(parent->top_left()) -
            parent->content_offset();
        spec.aux_rect = {local_top_left, {1, 1}};
        spec.placement_hints = MirPlacementHints{};
        spec.surface_placement_gravity = mir_placement_gravity_northwest;
        spec.aux_rect_placement_gravity = mir_placement_gravity_northwest;
    }
    else
    {
        spec.top_left = top_left;
    }
}

template<typename T>
auto property_handler(
    std::shared_ptr<mf::XCBConnection> const& connection,
    xcb_window_t window,
    xcb_atom_t property,
    std::function<void(T)>&& handler,
    std::function<void()>&& on_error = [](){}) -> std::pair<xcb_atom_t, std::function<std::function<void()>()>>
{
    return std::make_pair(
        property,
        [connection, window, property, handler = move(handler), on_error = move(on_error)]()
        {
            return connection->read_property(window, property, handler, on_error);
        });
}
}

struct mf::XWaylandSurface::InitialWlSurfaceData
{
    std::vector<shell::StreamSpecification> streams;
    std::vector<geom::Rectangle> input_shape;
};

mf::XWaylandSurface::XWaylandSurface(
    XWaylandWM *wm,
    std::shared_ptr<XCBConnection> const& connection,
    WlSeat& seat,
    std::shared_ptr<shell::Shell> const& shell,
    xcb_create_notify_event_t *event)
    : xwm(wm),
      connection{connection},
      seat(seat),
      shell{shell},
      window(event->window),
      property_handlers{
          property_handler<std::string const&>(
              connection,
              window,
              XCB_ATOM_WM_CLASS,
              [this](auto value)
              {
                  std::lock_guard<std::mutex> lock{mutex};
                  this->pending_spec(lock).application_id = value;
              }),
          property_handler<std::string const&>(
              connection,
              window,
              XCB_ATOM_WM_NAME,
              [this](auto value)
              {
                  std::lock_guard<std::mutex> lock{mutex};
                  this->pending_spec(lock).name = value;
              }),
          property_handler<std::string const&>(
              connection,
              window,
              connection->net_wm_name,
              [this](auto value)
              {
                  std::lock_guard<std::mutex> lock{mutex};
                  this->pending_spec(lock).name = value;
              }),
          property_handler<xcb_window_t>(
              connection,
              window,
              connection->wm_transient_for,
              [this](xcb_window_t value)
              {
                  std::shared_ptr<scene::Surface> parent_scene_surface; // May remain nullptr

                  if (auto const parent_surface = this->xwm->get_wm_surface(value))
                  {
                      std::lock_guard<std::mutex> parent_lock{parent_surface.value()->mutex};
                      parent_scene_surface = parent_surface.value()->weak_scene_surface.lock();
                  }

                  {
                      std::lock_guard<std::mutex> lock{this->mutex};
                      this->pending_spec(lock).parent = parent_scene_surface;
                      set_position(parent_scene_surface, this->latest_position, this->pending_spec(lock));
                  }
              },
              [this]()
              {
                  std::lock_guard<std::mutex> lock{mutex};
                  this->pending_spec(lock).parent = std::weak_ptr<scene::Surface>{};
              }),
          property_handler<std::vector<xcb_atom_t> const&>(
              connection,
              window,
              connection->wm_protocols,
              [this](std::vector<xcb_atom_t> const& value)
              {
                  std::lock_guard<std::mutex> lock{mutex};
                  this->supported_wm_protocols = std::set<xcb_atom_t>{value.begin(), value.end()};
              },
              [this]()
              {
                  std::lock_guard<std::mutex> lock{mutex};
                  this->supported_wm_protocols.clear();
              })},
      latest_size{event->width, event->height},
      latest_position{event->x, event->y}
{
    uint32_t const value = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE;
    xcb_change_window_attributes(*connection, window, XCB_CW_EVENT_MASK, &value);
}

mf::XWaylandSurface::~XWaylandSurface()
{
    close();
}

void mf::XWaylandSurface::map()
{
    WindowState state;
    {
        std::lock_guard<std::mutex> lock{mutex};
        state = window_state;
    }

    uint32_t const workspace = 1;
    connection->set_property<XCBType::CARDINAL32>(
        window,
        connection->net_wm_desktop,
        workspace);

    state.withdrawn = false;
    inform_client_of_window_state(state);
    request_scene_surface_state(state.mir_window_state());
    xcb_map_window(*connection, window);
    connection->flush();
}

void mf::XWaylandSurface::close()
{
    WindowState state;
    std::shared_ptr<scene::Surface> scene_surface;
    std::shared_ptr<XWaylandSurfaceObserver> observer;

    {
        std::lock_guard<std::mutex> lock{mutex};

        state = window_state;

        scene_surface = weak_scene_surface.lock();
        weak_scene_surface.reset();

        weak_session.reset();

        if (surface_observer)
        {
            observer = surface_observer.value();
        }
        surface_observer = std::experimental::nullopt;

        initial_wl_surface_data = std::experimental::nullopt;
    }

    connection->delete_property(window, connection->net_wm_desktop);

    state.withdrawn = true;
    inform_client_of_window_state(state);

    xcb_unmap_window(*connection, window);
    connection->flush();

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

void mf::XWaylandSurface::configure_request(xcb_configure_request_event_t* event)
{
    std::shared_ptr<scene::Surface> scene_surface;

    {
        std::lock_guard<std::mutex> lock{mutex};
        scene_surface = weak_scene_surface.lock();
    }

    if (scene_surface)
    {
        auto const content_offset = scene_surface->content_offset();

        geom::Point const old_position{scene_surface->top_left() + content_offset};
        geom::Point const new_position{
            event->value_mask & XCB_CONFIG_WINDOW_X ? geom::X{event->x} : old_position.x,
            event->value_mask & XCB_CONFIG_WINDOW_Y ? geom::Y{event->y} : old_position.y,
        };

        geom::Size const old_size{scene_surface->content_size()};
        geom::Size const new_size{
            event->value_mask & XCB_CONFIG_WINDOW_WIDTH ? geom::Width{event->width} : old_size.width,
            event->value_mask & XCB_CONFIG_WINDOW_HEIGHT ? geom::Height{event->height} : old_size.height,
        };

        shell::SurfaceSpecification mods;

        if (old_position != new_position)
        {
            set_position(scene_surface->parent(), new_position - content_offset, mods);
        }

        if (old_size != new_size)
        {
            // Mir appears to not respect size request unless both width and height are set
            mods.width = new_size.width;
            mods.height = new_size.height;
        }

        if (!mods.is_empty())
        {
            shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
        }
    }
    else
    {
        connection->configure_window(
            window,
            geom::Point{event->x, event->y},
            geom::Size{event->width, event->height});
    }
}

void mf::XWaylandSurface::configure_notify(xcb_configure_notify_event_t* event)
{
    std::lock_guard<std::mutex> lock{mutex};
    latest_position = geom::Point{event->x, event->y},
    latest_size = geom::Size{event->width, event->height};
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

                if (property == connection->net_wm_state_hidden)
                    prop_ptr = &new_window_state.minimized;
                else if (property == connection->net_wm_state_maximized_horz) // assume vert is also set
                    prop_ptr = &new_window_state.maximized;
                else if (property == connection->net_wm_state_fullscreen)
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

    inform_client_of_window_state(new_window_state);
    request_scene_surface_state(new_window_state.mir_window_state());
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
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "WM_CHANGE_STATE client message sent invalid state " +
                std::to_string(static_cast<std::underlying_type<WmState>::type>(requested_state))));
        }
    }

    inform_client_of_window_state(new_window_state);
    request_scene_surface_state(new_window_state.mir_window_state());
}

void mf::XWaylandSurface::property_notify(xcb_atom_t property)
{
    auto const handler = property_handlers.find(property);
    if (handler != property_handlers.end())
    {
        std::shared_ptr<scene::Surface> scene_surface;
        std::experimental::optional<std::unique_ptr<shell::SurfaceSpecification>> spec;

        {
            std::lock_guard<std::mutex> lock{mutex};
            scene_surface = weak_scene_surface.lock();
            spec = consume_pending_spec(lock);
        }

        if (scene_surface)
        {
            auto completion = handler->second();
            completion();
            if (spec)
            {
                shell->modify_surface(scene_surface->session().lock(), scene_surface, *spec.value());
            }
        }
    }
}

void mf::XWaylandSurface::attach_wl_surface(WlSurface* wl_surface)
{
    // We assume we are on the Wayland thread

    auto const observer = std::make_shared<XWaylandSurfaceObserver>(seat, wl_surface, this);

    {
        std::lock_guard<std::mutex> lock{mutex};

        if (surface_observer || weak_session.lock() || initial_wl_surface_data)
            BOOST_THROW_EXCEPTION(std::runtime_error("XWaylandSurface::set_wl_surface() called multiple times"));

        surface_observer = observer;

        weak_session = get_session(wl_surface->resource);

        initial_wl_surface_data = std::make_unique<InitialWlSurfaceData>();
        wl_surface->populate_surface_data(
            initial_wl_surface_data.value()->streams,
            initial_wl_surface_data.value()->input_shape,
            {});
    }

    // If a buffer has alread been committed, we need to create the scene::Surface without waiting for next commit
    if (wl_surface->buffer_size())
        create_scene_surface_if_needed();
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

auto mf::XWaylandSurface::WindowState::operator==(WindowState const& that) const -> bool
{
    return
        withdrawn == that.withdrawn &&
        minimized == that.minimized &&
        maximized == that.maximized &&
        fullscreen == that.fullscreen;
}

auto mf::XWaylandSurface::WindowState::mir_window_state() const -> MirWindowState
{
    // withdrawn is ignored
    if (minimized)
        return mir_window_state_minimized;
    else if (fullscreen)
        return mir_window_state_fullscreen;
    else if (maximized)
        return mir_window_state_maximized;
    else
        return mir_window_state_restored;
}

auto mf::XWaylandSurface::WindowState::updated_from(MirWindowState state) const -> WindowState
{
    auto updated = *this;

    // If there is a MirWindowState to update from, the surface should not be withdrawn
    updated.withdrawn = false;

    switch (state)
    {
    case mir_window_state_hidden:
    case mir_window_state_minimized:
        updated.minimized = true;
        // don't change maximized or fullscreen
        break;

    case mir_window_state_fullscreen:
        updated.minimized = false;
        updated.fullscreen = true;
        // don't change maximizeds
        break;

    case mir_window_state_maximized:
    case mir_window_state_vertmaximized:
    case mir_window_state_horizmaximized:
        updated.minimized = false;
        updated.maximized = true;
        updated.fullscreen = false;
        break;

    case mir_window_state_restored:
    case mir_window_state_unknown:
    case mir_window_state_attached:
        updated.minimized = false;
        updated.maximized = false;
        updated.fullscreen = false;
        break;

    case mir_window_states:
        break;
    }

    return updated;
}

void mf::XWaylandSurface::scene_surface_state_set(MirWindowState new_state)
{
    WindowState state;
    {
        std::lock_guard<std::mutex> lock{mutex};
        state = window_state.updated_from(new_state);
    }
    inform_client_of_window_state(state);
}

void mf::XWaylandSurface::scene_surface_resized(geometry::Size const& new_size)
{
    connection->configure_window(window, std::experimental::nullopt, new_size);
    connection->flush();
}

void mf::XWaylandSurface::scene_surface_moved_to(geometry::Point const& new_top_left)
{
    std::shared_ptr<scene::Surface> scene_surface;
    {
        std::lock_guard<std::mutex> lock{mutex};
        scene_surface = weak_scene_surface.lock();
    }
    auto const content_offset = scene_surface ? scene_surface->content_offset() : geom::Displacement{};
    connection->configure_window(window, new_top_left + content_offset, std::experimental::nullopt);
    connection->flush();
}

void mf::XWaylandSurface::scene_surface_close_requested()
{
    xcb_destroy_window(*connection, window);
    connection->flush();
}

void mf::XWaylandSurface::run_on_wayland_thread(std::function<void()>&& work)
{
    xwm->run_on_wayland_thread(std::move(work));
}

void mf::XWaylandSurface::wl_surface_destroyed()
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("%s's wl_surface destoyed", connection->window_debug_string(window).c_str());
    }
    close();
}

void mf::XWaylandSurface::wl_surface_committed()
{
    create_scene_surface_if_needed();
}

auto mf::XWaylandSurface::scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>>
{
    std::lock_guard<std::mutex> lock{mutex};
    if (auto const scene_surface = weak_scene_surface.lock())
        return scene_surface;
    else
        return std::experimental::nullopt;
}

auto mf::XWaylandSurface::pending_spec(std::lock_guard<std::mutex> const&) -> msh::SurfaceSpecification&
{
    if (!nullable_pending_spec)
        nullable_pending_spec = std::make_unique<msh::SurfaceSpecification>();
    return *nullable_pending_spec;
}

auto mf::XWaylandSurface::consume_pending_spec(
    std::lock_guard<std::mutex> const&) -> std::experimental::optional<std::unique_ptr<msh::SurfaceSpecification>>
{
    if (nullable_pending_spec)
        return move(nullable_pending_spec);
    else
        return std::experimental::nullopt;
}

void mf::XWaylandSurface::create_scene_surface_if_needed()
{
    WindowState state;
    scene::SurfaceCreationParameters params;
    std::shared_ptr<scene::SurfaceObserver> observer;
    std::shared_ptr<scene::Session> session;

    {
        std::lock_guard<std::mutex> lock{mutex};

        session = weak_session.lock();

        if (weak_scene_surface.lock() ||
            !initial_wl_surface_data ||
            !surface_observer ||
            !session)
        {
            // surface is already created, being created or not ready to be created
            return;
        }

        if (verbose_xwayland_logging_enabled())
        {
            log_debug("creating scene surface for %s", connection->window_debug_string(window).c_str());
        }

        state = window_state;
        state.withdrawn = false;

        observer = surface_observer.value();

        params.streams = std::move(initial_wl_surface_data.value()->streams);
        params.input_shape = std::move(initial_wl_surface_data.value()->input_shape);
        initial_wl_surface_data = std::experimental::nullopt;

        params.size = latest_size;
        params.top_left = latest_position;
        params.type = mir_window_type_freestyle;
        params.state = state.mir_window_state();
    }

    std::vector<std::function<void()>> reply_functions;

    auto const window_attrib_cookie = xcb_get_window_attributes(*connection, window);
    reply_functions.push_back([this, &params, window_attrib_cookie]
        {
            auto const reply = xcb_get_window_attributes_reply(*connection, window_attrib_cookie, nullptr);
            if (reply)
            {
                params.server_side_decorated = !reply->override_redirect;
                free(reply);
            }
        });

    // Read all properties
    for (auto const& handler : property_handlers)
    {
        reply_functions.push_back(handler.second());
    }

    // Wait for and process all the XCB replies
    for (auto const& reply_function : reply_functions)
    {
        reply_function();
    }

    // property_handlers will have updated the pending spec. Use it.
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (auto const spec = consume_pending_spec(lock))
        {
            params.update_from(*spec.value());
        }
    }

    auto const surface = shell->create_surface(session, params, observer);
    inform_client_of_window_state(state);
    connection->configure_window(
        window,
        surface->top_left() + surface->content_offset(),
        surface->content_size());

    {
        std::lock_guard<std::mutex> lock{mutex};
        weak_scene_surface = surface;
    }
}

void mf::XWaylandSurface::inform_client_of_window_state(WindowState const& new_window_state)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("inform_client_of_window_state(window:     %s", connection->window_debug_string(window).c_str());
        log_debug("                              withdrawn:  %s", new_window_state.withdrawn ? "yes" : "no");
        log_debug("                              minimized:  %s", new_window_state.minimized ? "yes" : "no");
        log_debug("                              maximized:  %s", new_window_state.maximized ? "yes" : "no");
        log_debug("                              fullscreen: %s)", new_window_state.fullscreen ? "yes" : "no");
    }

    {
        std::lock_guard<std::mutex> lock{mutex};

        if (new_window_state == window_state)
            return;

        window_state = new_window_state;
    }

    WmState wm_state;

    if (new_window_state.withdrawn)
        wm_state = WmState::WITHDRAWN;
    else if (new_window_state.minimized)
        wm_state = WmState::ICONIC;
    else
        wm_state = WmState::NORMAL;

    uint32_t const wm_state_properties[]{
        static_cast<uint32_t>(wm_state),
        XCB_WINDOW_NONE // Icon window
    };
    connection->set_property<XCBType::WM_STATE>(window, connection->wm_state, wm_state_properties);

    if (new_window_state.withdrawn)
    {
        xcb_delete_property(
            *connection,
            window,
            connection->net_wm_state);
    }
    else
    {
        std::vector<xcb_atom_t> net_wm_states;

        if (new_window_state.minimized)
        {
            net_wm_states.push_back(connection->net_wm_state_hidden);
        }
        if (new_window_state.maximized)
        {
            net_wm_states.push_back(connection->net_wm_state_maximized_horz);
            net_wm_states.push_back(connection->net_wm_state_maximized_vert);
        }
        if (new_window_state.fullscreen)
        {
            net_wm_states.push_back(connection->net_wm_state_fullscreen);
        }
        // TODO: Set _NET_WM_STATE_MODAL if appropriate

        connection->set_property<XCBType::ATOM>(window, connection->net_wm_state, net_wm_states);
    }

    connection->flush();
}

void mf::XWaylandSurface::request_scene_surface_state(MirWindowState new_state)
{
    std::shared_ptr<scene::Surface> scene_surface;

    {
        std::lock_guard<std::mutex> lock{mutex};
        scene_surface = weak_scene_surface.lock();
    }

    if (scene_surface && scene_surface->state() != new_state)
    {
        shell::SurfaceSpecification mods;
        mods.state = new_state;
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
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
