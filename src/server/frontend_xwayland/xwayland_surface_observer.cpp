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

#include "xwayland_surface_observer.h"
#include "xwayland_surface_observer_surface.h"
#include "xwayland_surface.h"
#include "wl_seat.h"
#include "wayland_utils.h"
#include "window_wl_surface_role.h"
#include "wayland_input_dispatcher.h"

#include <mir/executor.h>
#include <mir/events/event_helpers.h>
#include <mir/events/input_event.h>

#include <mir/log.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mev = mir::events;
namespace mi = mir::input;

namespace
{
/// Returns true if this input event could potentially start a move/resize
auto is_move_resize_event(MirInputEvent const* event) -> bool
{
    switch (mir_input_event_get_type(event))
    {
    case mir_input_event_type_pointer:
        switch (mir_pointer_event_action(mir_input_event_get_pointer_event(event)))
        {
        case mir_pointer_action_button_up:
        case mir_pointer_action_button_down:
        case mir_pointer_action_enter:
            return true;

        default:
            return false;
        }

    case mir_input_event_type_touch:
    {
        auto const touch_ev = mir_input_event_get_touch_event(event);
        for (unsigned i = 0; i < mir_touch_event_point_count(touch_ev); i++)
        {
            switch (mir_touch_event_action(touch_ev, i))
            {
            case mir_touch_action_down:
                return true;

            default:
                break;
            }
        }
        return false;
    }

    case mir_input_event_type_key:
    case mir_input_event_type_keyboard_resync:
    case mir_input_event_types:
        return false;
    }
    return false; // make compiler happy
}
}

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
    std::lock_guard lock{input_dispatcher->mutex};
    input_dispatcher->dispatcher = std::nullopt;
}

auto mf::XWaylandSurfaceObserver::latest_move_resize_event() -> std::shared_ptr<MirInputEvent const>
{
    return *latest_move_resize_event_.lock();
}

void mf::XWaylandSurfaceObserver::attrib_changed(ms::Surface const*, MirWindowAttrib attrib, int value)
{
    switch (attrib)
    {
    case mir_window_attrib_focus:
    {
        auto state = static_cast<MirWindowFocusState>(value);
        wm_surface->scene_surface_focus_set(state != mir_window_focus_state_unfocused);
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

void mf::XWaylandSurfaceObserver::input_consumed(ms::Surface const*, std::shared_ptr<MirEvent const> const& event)
{
    if (mir_event_get_type(event.get()) == mir_event_type_input)
    {
        // Must clone the event so we can scale the positions to XWayland scale
        auto const owned_event = std::dynamic_pointer_cast<MirInputEvent>(
            std::shared_ptr<MirEvent>{mev::clone_event(*event)});
        mev::scale_local_position(*owned_event, scale);

        if (is_move_resize_event(mir_event_get_input_event(owned_event.get())))
        {
            *latest_move_resize_event_.lock() = owned_event;
        }

        aquire_input_dispatcher(
            [owned_event](auto input_dispatcher)
            {
                input_dispatcher->handle_event(owned_event);
            });
    }
}

mf::XWaylandSurfaceObserver::ThreadsafeInputDispatcher::ThreadsafeInputDispatcher(
    std::unique_ptr<WaylandInputDispatcher> dispatcher)
    : dispatcher{std::move(dispatcher)}
{
}

mf::XWaylandSurfaceObserver::ThreadsafeInputDispatcher::~ThreadsafeInputDispatcher()
{
}

void mf::XWaylandSurfaceObserver::aquire_input_dispatcher(std::function<void(WaylandInputDispatcher*)>&& work)
{
    wayland_executor.spawn([work = std::move(work), input_dispatcher = input_dispatcher]()
        {
            std::lock_guard lock{input_dispatcher->mutex};
            if (input_dispatcher->dispatcher)
            {
                work(input_dispatcher->dispatcher.value().get());
            }
        });
}
