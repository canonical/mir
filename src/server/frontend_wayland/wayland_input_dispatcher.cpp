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
 */

#include "wayland_input_dispatcher.h"
#include "wl_surface.h"
#include "wl_seat.h"
#include "wl_pointer.h"
#include "wl_keyboard.h"
#include "wl_touch.h"

#include <mir/input/keymap.h>
#include <mir/events/pointer_event.h>
#include <mir/events/touch_event.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mi = mir::input;
namespace mw = mir::wayland;

mf::WaylandInputDispatcher::WaylandInputDispatcher(
    WlSeat* seat,
    WlSurface* wl_surface)
    : seat{seat},
      client{wl_surface->client},
      wl_surface{mw::make_weak(wl_surface)}
{
}

void mf::WaylandInputDispatcher::handle_event(std::shared_ptr<MirEvent const> const& event)
{
    auto const input_ev = mir_event_get_input_event(event.get());

    if (!wl_surface || !input_ev)
    {
        return;
    }

    // Remember the timestamp of any events "signed" with a cookie
    if (mir_input_event_has_cookie(input_ev))
    {
        timestamp = std::chrono::nanoseconds{mir_input_event_get_event_time(input_ev)};
    }

    switch (mir_input_event_get_type(input_ev))
    {

    case mir_input_event_type_pointer:
    {
        auto const pointer_event = dynamic_pointer_cast<MirPointerEvent const>(event);
        seat->for_each_listener(client, [&](WlPointer* pointer)
            {
                pointer->event(pointer_event, wl_surface.value());
            });
    }   break;

    case mir_input_event_type_touch:
    {
        auto const touch_event = dynamic_pointer_cast<MirTouchEvent const>(event);
        seat->for_each_listener(client, [&](WlTouch* touch)
            {
                touch->event(touch_event, wl_surface.value());
            });
    }   break;

    // Keyboard events are sent to the WlSeat via it's KeyboardObserver

    default:
        break;
    }
}
