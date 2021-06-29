/*
 * Copyright © 2020 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "xwayland_surface_observer.h"
#include "xwayland_surface_observer_surface.h"
#include "xwayland_surface.h"
#include "wl_seat.h"
#include "wayland_utils.h"
#include "window_wl_surface_role.h"
#include "wayland_input_dispatcher.h"

#include <mir/executor.h>
#include <mir/events/event_builders.h>

#include <mir/log.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mev = mir::events;
namespace mi = mir::input;

mf::XWaylandSurfaceObserver::XWaylandSurfaceObserver(
    Executor& wayland_executor,
    WlSeat& seat,
    WlSurface* wl_surface,
    XWaylandSurfaceObserverSurface* wm_surface,
    float scale)
    : wm_surface{wm_surface},
      wayland_executor{wayland_executor},
      input_dispatcher{std::make_shared<ThreadsafeInputDispatcher>(
          std::make_unique<WaylandInputDispatcher>(&seat, wl_surface))},
      scale{scale}
{
}

mf::XWaylandSurfaceObserver::~XWaylandSurfaceObserver()
{
    std::lock_guard<std::mutex> lock{input_dispatcher->mutex};
    input_dispatcher->dispatcher = std::experimental::nullopt;
}

void mf::XWaylandSurfaceObserver::attrib_changed(ms::Surface const*, MirWindowAttrib attrib, int value)
{
    switch (attrib)
    {
    case mir_window_attrib_focus:
    {
        auto has_focus = static_cast<bool>(value);
        wm_surface->scene_surface_focus_set(has_focus);
        aquire_input_dispatcher(
            [has_focus](auto input_dispatcher)
            {
                input_dispatcher->set_focus(has_focus);
            });
    }   break;

    case mir_window_attrib_state:
    {
        auto state = static_cast<MirWindowState>(value);
        wm_surface->scene_surface_state_set(state);
    }   break;

    default:;
    }
}

void mf::XWaylandSurfaceObserver::content_resized_to(ms::Surface const*, geom::Size const& content_size)
{
    wm_surface->scene_surface_resized(content_size * scale);
}

void mf::XWaylandSurfaceObserver::moved_to(ms::Surface const*, geom::Point const& top_left)
{
    wm_surface->scene_surface_moved_to(as_point(as_displacement(top_left) * scale));
}

void mf::XWaylandSurfaceObserver::client_surface_close_requested(ms::Surface const*)
{
    wm_surface->scene_surface_close_requested();
}

void mf::XWaylandSurfaceObserver::input_consumed(ms::Surface const*, MirEvent const* event)
{
    if (mir_event_get_type(event) == mir_event_type_input)
    {
        std::shared_ptr<MirEvent> owned_event = mev::clone_event(*event);
        mev::scale_positions(*owned_event, scale);

        aquire_input_dispatcher(
            [owned_event](auto input_dispatcher)
            {
                auto const input_event = mir_event_get_input_event(owned_event.get());
                input_dispatcher->handle_event(input_event);
            });
    }
}

auto mf::XWaylandSurfaceObserver::latest_timestamp() const -> std::chrono::nanoseconds
{
    std::lock_guard<std::mutex> lock{input_dispatcher->mutex};
    if (input_dispatcher->dispatcher)
        return input_dispatcher->dispatcher.value()->latest_timestamp();
    else
        return {};
}

mf::XWaylandSurfaceObserver::ThreadsafeInputDispatcher::ThreadsafeInputDispatcher(
    std::unique_ptr<WaylandInputDispatcher> dispatcher)
    : dispatcher{move(dispatcher)}
{
}

mf::XWaylandSurfaceObserver::ThreadsafeInputDispatcher::~ThreadsafeInputDispatcher()
{
}

void mf::XWaylandSurfaceObserver::aquire_input_dispatcher(std::function<void(WaylandInputDispatcher*)>&& work)
{
    wayland_executor.spawn([work = move(work), input_dispatcher = input_dispatcher]()
        {
            std::lock_guard<std::mutex> lock{input_dispatcher->mutex};
            if (input_dispatcher->dispatcher)
            {
                work(input_dispatcher->dispatcher.value().get());
            }
        });
}
