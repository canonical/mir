/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wayland_surface_observer.h"
#include "wayland_utils.h"
#include "window_wl_surface_role.h"
#include "wl_surface.h"

#include <mir/executor.h>
#include <mir/log.h>
#include <mir/events/input_event.h>
#include <mir/wayland/client.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mi = mir::input;
namespace mw = mir::wayland;

mf::WaylandSurfaceObserver::WaylandSurfaceObserver(
    Executor& wayland_executor,
    WlSeat* seat,
    WlSurface* surface,
    WindowWlSurfaceRole* window)
    : wayland_executor{wayland_executor},
      impl{std::make_shared<Impl>(
          mw::make_weak(window),
          std::make_unique<WaylandInputDispatcher>(seat, surface))}
{
}

mf::WaylandSurfaceObserver::~WaylandSurfaceObserver()
{
}

void mf::WaylandSurfaceObserver::attrib_changed(ms::Surface const*, MirWindowAttrib attrib, int value)
{
    switch (attrib)
    {
    case mir_window_attrib_focus:
        run_on_wayland_thread_unless_window_destroyed(
            [value](Impl* /*impl*/, WindowWlSurfaceRole* window)
            {
                auto const state = static_cast<MirWindowFocusState>(value);
                window->handle_active_change(state != mir_window_focus_state_unfocused);
            });
        break;

    case mir_window_attrib_state:
        run_on_wayland_thread_unless_window_destroyed(
            [value](Impl* impl, WindowWlSurfaceRole* window)
            {
                impl->current_state = static_cast<MirWindowState>(value);
                window->handle_state_change(impl->current_state);
            });
        break;

    default:;
    }
}

void mf::WaylandSurfaceObserver::content_resized_to(ms::Surface const*, geom::Size const& content_size)
{
    run_on_wayland_thread_unless_window_destroyed(
        [content_size](Impl* impl, WindowWlSurfaceRole* window)
        {
            if (impl->requested_size && impl->requested_size == content_size)
            {
                // We've already requested this size, no action required
                return;
            }

            if (content_size == impl->window_size)
            {
                // The window manager is accepting the client's window size, no action required
                return;
            }

            // The window manager is changing the client's window size, tell the client
            impl->requested_size = content_size;
            window->handle_resize(std::nullopt, content_size);
        });
}

void mf::WaylandSurfaceObserver::client_surface_close_requested(ms::Surface const*)
{
    run_on_wayland_thread_unless_window_destroyed(
        [](Impl*, WindowWlSurfaceRole* window)
        {
            window->handle_close_request();
        });
}

void mf::WaylandSurfaceObserver::placed_relative(ms::Surface const*, geometry::Rectangle const& placement)
{
    run_on_wayland_thread_unless_window_destroyed(
        [placement = placement](Impl* impl, WindowWlSurfaceRole* window)
        {
            impl->requested_size = placement.size;
            window->handle_resize(placement.top_left, placement.size);
        });
}

void mf::WaylandSurfaceObserver::input_consumed(ms::Surface const*, std::shared_ptr<MirEvent const> const& event)
{
    if (mir_event_get_type(event.get()) == mir_event_type_input)
    {
        run_on_wayland_thread_unless_window_destroyed(
            [event](Impl* impl, WindowWlSurfaceRole*)
            {
                impl->input_dispatcher->handle_event(std::dynamic_pointer_cast<MirInputEvent const>(event));
            });
    }
}

void mf::WaylandSurfaceObserver::entered_output(ms::Surface const*, graphics::DisplayConfigurationOutputId const& id)
{
    run_on_wayland_thread_unless_window_destroyed(
        [id](Impl* impl, WindowWlSurfaceRole*)
        {
            impl->window.value().handle_enter_output(id);
        });
}

void mf::WaylandSurfaceObserver::left_output(ms::Surface const*, graphics::DisplayConfigurationOutputId const& id)
{
    run_on_wayland_thread_unless_window_destroyed(
        [id](Impl* impl, WindowWlSurfaceRole*)
        {
            impl->window.value().handle_leave_output(id);
        });
}

void mf::WaylandSurfaceObserver::rescale_output(ms::Surface const*, graphics::DisplayConfigurationOutputId const& id)
{
    run_on_wayland_thread_unless_window_destroyed(
        [id](Impl* impl, WindowWlSurfaceRole*)
        {
            impl->window.value().handle_scale_output(id);
        });
}

void mir::frontend::WaylandSurfaceObserver::tiled_edges(scene::Surface const*, Flags<MirTiledEdge> edges)
{
    run_on_wayland_thread_unless_window_destroyed(
        [edges](Impl* impl, WindowWlSurfaceRole*)
        {
            impl->window.value().handle_tiled_edges(edges);
        });
}

void mf::WaylandSurfaceObserver::run_on_wayland_thread_unless_window_destroyed(
    std::function<void(Impl* impl, WindowWlSurfaceRole* window)>&& work)
{
    wayland_executor.spawn(
        [impl=impl, work=std::move(work)]
        {
            if (impl->window)
            {
                work(impl.get(), &impl->window.value());
            }
        });
}
