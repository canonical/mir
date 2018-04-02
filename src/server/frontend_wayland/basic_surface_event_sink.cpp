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
#include "wl_pointer.h"
#include "wl_keyboard.h"
#include "wl_touch.h"
#include "wayland_utils.h"

namespace mf = mir::frontend;

mf::BasicSurfaceEventSink::BasicSurfaceEventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* event_sink)
    : seat{seat},
      client{client},
      target{target},
      event_sink{event_sink},
      window_size{geometry::Size{0,0}},
      destroyed{std::make_shared<bool>(false)}
{
}

mf::BasicSurfaceEventSink::~BasicSurfaceEventSink()
{
    *destroyed = true;
}

void mf::BasicSurfaceEventSink::handle_event(EventUPtr&& event)
{
    switch (mir_event_get_type(event.get()))
    {
        case mir_event_type_resize:
        case mir_event_type_input:
        case mir_event_type_keymap:
        case mir_event_type_window:
            seat->spawn(run_unless(
                destroyed,
                [this, event = std::make_shared<EventUPtr>(move(event))]()
                {
                    handle_event_on_wayland_thread(move(*event));
                }));
            break;
        default:
            break;
    }
}

void mf::BasicSurfaceEventSink::handle_event_on_wayland_thread(EventUPtr&& event)
{
    // NOTE: to add a type of event we care about here, you must add it to this switch statement as well as the one in
    //       handle_event()
    switch (mir_event_get_type(event.get()))
    {
    case mir_event_type_resize:
    {
        auto* const resize_event = mir_event_get_resize_event(event.get());
        requested_size = {mir_resize_event_get_width(resize_event), mir_resize_event_get_height(resize_event)};
        if (requested_size != window_size)
            send_resize(requested_size);
        break;
    }
    case mir_event_type_input:
    {
        auto input_event = mir_event_get_input_event(event.get());

        // Remember the timestamp of any events "signed" with a cookie
        if (mir_input_event_has_cookie(input_event))
            timestamp_ns = mir_input_event_get_event_time(input_event);

        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_key:
            seat->for_each_listener(client, [this, input_event](WlKeyboard* keyboard)
                {
                    keyboard->handle_event(input_event, target);
                });
            break;
        case mir_input_event_type_pointer:
            seat->for_each_listener(client, [this, input_event](WlPointer* pointer)
                {
                    pointer->handle_event(input_event, target);
                });
            break;
        case mir_input_event_type_touch:
            seat->for_each_listener(client, [this, input_event](WlTouch* touch)
                {
                    touch->handle_event(input_event, target);
                });
            break;
        default:
            break;
        }
        break;
    }
    case mir_event_type_keymap:
    {
        auto const map_ev = mir_event_get_keymap_event(event.get());

        seat->for_each_listener(client, [this, map_ev](WlKeyboard* keyboard)
            {
                keyboard->handle_event(map_ev, target);
            });
        break;
    }
    case mir_event_type_window:
    {
        auto const wev = mir_event_get_window_event(event.get());

        switch (mir_window_event_get_attribute(wev))
        {
        case mir_window_attrib_focus:
            has_focus = mir_window_event_get_attribute_value(wev);
            send_resize(requested_size);
            break;

        case mir_window_attrib_state:
            current_state = MirWindowState(mir_window_event_get_attribute_value(wev));
            send_resize(requested_size);
            break;

        default:;
        }

        seat->for_each_listener(client, [this, wev](WlKeyboard* keyboard)
            {
                keyboard->handle_event(wev, target);
            });
        break;
    }
    default:
        break;
    }
}
