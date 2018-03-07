/*
 * Copyright © 2018 Canonical Ltd.
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
 */

#include "basic_surface_event_sink.h"
#include "wl_seat.h"

namespace mf = mir::frontend;

void mf::BasicSurfaceEventSink::handle_event(MirEvent const& event)
{
    switch (mir_event_get_type(&event))
    {
    case mir_event_type_resize:
    {
        auto* const resize_event = mir_event_get_resize_event(&event);
        geometry::Size const size{mir_resize_event_get_width(resize_event), mir_resize_event_get_height(resize_event)};
        if (size != window_size)
            send_resize(size);
        break;
    }
    case mir_event_type_input:
    {
        auto input_event = mir_event_get_input_event(&event);

        // Remember the timestamp of any events "signed" with a cookie
        if (mir_input_event_has_cookie(input_event))
            timestamp = mir_input_event_get_event_time(input_event);

        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_key:
            seat->handle_keyboard_event(client, input_event, target);
            break;
        case mir_input_event_type_pointer:
            seat->handle_pointer_event(client, input_event, target);
            break;
        case mir_input_event_type_touch:
            seat->handle_touch_event(client, input_event, target);
            break;
        default:
            break;
        }
        break;
    }
    case mir_event_type_keymap:
    {
        auto const map_ev = mir_event_get_keymap_event(&event);

        seat->handle_event(client, map_ev, target);
        break;
    }
    case mir_event_type_window:
    {
        auto const wev = mir_event_get_window_event(&event);

        seat->handle_event(client, wev, target);
    }
    default:
        break;
    }
}
