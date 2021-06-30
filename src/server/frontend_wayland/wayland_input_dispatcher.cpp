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

#include "wayland_input_dispatcher.h"
#include "wl_surface.h"
#include "wl_seat.h"
#include "wl_pointer.h"
#include "wl_keyboard.h"
#include "wl_touch.h"

#include <mir/input/keymap.h>
#include <mir/log.h>

#include <linux/input-event-codes.h>
#include <boost/throw_exception.hpp>

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

void mf::WaylandInputDispatcher::set_focus(bool has_focus)
{
    if (!wl_surface)
    {
        return;
    }

    if (has_focus)
        seat->notify_focus(client);

    seat->for_each_listener(client, [&](WlKeyboard* keyboard)
        {
            keyboard->focussed(wl_surface.value(), has_focus);
        });
}

void mf::WaylandInputDispatcher::handle_event(MirInputEvent const* event)
{
    if (!wl_surface)
    {
        return;
    }

    // Remember the timestamp of any events "signed" with a cookie
    if (mir_input_event_has_cookie(event))
    {
        timestamp = std::chrono::nanoseconds{mir_input_event_get_event_time(event)};
    }

    switch (mir_input_event_get_type(event))
    {
    case mir_input_event_type_key:
    {
        auto const keyboard_event = mir_input_event_get_keyboard_event(event);
        seat->for_each_listener(client, [&](WlKeyboard* keyboard)
            {
                keyboard->event(keyboard_event, wl_surface.value());
            });
    }   break;

    case mir_input_event_type_pointer:
    {
        auto const pointer_event = mir_input_event_get_pointer_event(event);
        seat->for_each_listener(client, [&](WlPointer* pointer)
            {
                pointer->event(pointer_event, wl_surface.value());
            });
    }   break;

    case mir_input_event_type_touch:
    {
        auto const touch_event = mir_input_event_get_touch_event(event);
        seat->for_each_listener(client, [&](WlTouch* touch)
            {
                touch->event(touch_event, wl_surface.value());
            });
    }   break;

    default:
        break;
    }
}
