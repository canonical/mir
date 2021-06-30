/*
 * Copyright © 2018-2019 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              William Wold <william.wold@canonical.com>
 */

#include "wayland_surface_observer.h"
#include "wayland_utils.h"
#include "window_wl_surface_role.h"

#include <mir/executor.h>
#include <mir/events/event_builders.h>
#include <mir/log.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mev = mir::events;
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
            [value](Impl* impl, WindowWlSurfaceRole* window)
            {
                auto has_focus = static_cast<bool>(value);
                impl->input_dispatcher->set_focus(has_focus);
                window->handle_active_change(has_focus);
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
            if (content_size != impl->window_size)
            {
                impl->requested_size = content_size;
                window->handle_resize(std::experimental::nullopt, content_size);
            }
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

void mf::WaylandSurfaceObserver::input_consumed(ms::Surface const*, MirEvent const* event)
{
    if (mir_event_get_type(event) == mir_event_type_input)
    {
        std::shared_ptr<MirEvent> owned_event = mev::clone_event(*event);

        run_on_wayland_thread_unless_window_destroyed(
            [owned_event](Impl* impl, WindowWlSurfaceRole*)
            {
                auto const input_event = mir_event_get_input_event(owned_event.get());
                impl->input_dispatcher->handle_event(input_event);
            });
    }
}

auto mf::WaylandSurfaceObserver::latest_timestamp() const -> std::chrono::nanoseconds
{
    return impl->input_dispatcher->latest_timestamp();
}

void mf::WaylandSurfaceObserver::run_on_wayland_thread_unless_window_destroyed(
    std::function<void(Impl* impl, WindowWlSurfaceRole* window)>&& work)
{
    wayland_executor.spawn(
        [impl=impl, work=move(work)]
        {
            if (impl->window)
            {
                work(impl.get(), &impl->window.value());
            }
        });
}
