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
#include "wl_surface.h"
#include "wayland_utils.h"

#include <linux/input-event-codes.h>

namespace mf = mir::frontend;
namespace geom = mir::geometry;

mf::BasicSurfaceEventSink::BasicSurfaceEventSink(WlSeat* seat, wl_client* client, wl_resource* target, WlAbstractMirWindow* window)
    : seat{seat},
      client{client},
      surface{WlSurface::from(target)},
      window{window},
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
    seat->spawn(run_unless(
        destroyed,
        [this, event = std::make_shared<EventUPtr>(move(event))]()
        {
            switch (mir_event_get_type(event->get()))
            {
                case mir_event_type_resize:
                    handle_resize_event(mir_event_get_resize_event(event->get()));
                    break;
                case mir_event_type_input:
                    handle_input_event(mir_event_get_input_event(event->get()));
                    break;
                case mir_event_type_keymap:
                    handle_keymap_event(mir_event_get_keymap_event(event->get()));
                    break;
                case mir_event_type_window:
                    handle_window_event(mir_event_get_window_event(event->get()));
                    break;
                default:
                    break;
            }
        }));
}

void mf::BasicSurfaceEventSink::handle_resize_event(MirResizeEvent const* event)
{
    requested_size = {mir_resize_event_get_width(event), mir_resize_event_get_height(event)};
    if (requested_size != window_size)
        window->handle_resize(requested_size);
}

void mf::BasicSurfaceEventSink::handle_input_event(MirInputEvent const* event)
{
    // Remember the timestamp of any events "signed" with a cookie
    if (mir_input_event_has_cookie(event))
        timestamp_ns = mir_input_event_get_event_time(event);

    switch (mir_input_event_get_type(event))
    {
    case mir_input_event_type_key:
        seat->for_each_listener(client, [this, event = mir_input_event_get_keyboard_event(event)](WlKeyboard* keyboard)
            {
                keyboard->handle_keyboard_event(event, surface);
            });
        break;
    case mir_input_event_type_pointer:
        seat->for_each_listener(client, [this, event = mir_input_event_get_pointer_event(event)](WlPointer* pointer)
            {
                pointer->handle_event(event, surface);
            });
        break;
    case mir_input_event_type_touch:
        seat->for_each_listener(client, [this, event = mir_input_event_get_touch_event(event)](WlTouch* touch)
            {
                touch->handle_event(event, surface);
            });
        break;
    default:
        break;
    }
}

void mf::BasicSurfaceEventSink::handle_keymap_event(MirKeymapEvent const* event)
{
    seat->for_each_listener(client, [this, event](WlKeyboard* keyboard)
        {
            keyboard->handle_keymap_event(event, surface);
        });
}

void mf::BasicSurfaceEventSink::handle_window_event(MirWindowEvent const* event)
{
    switch (mir_window_event_get_attribute(event))
    {
    case mir_window_attrib_focus:
        has_focus = mir_window_event_get_attribute_value(event);
        window->handle_resize(requested_size);
        break;

    case mir_window_attrib_state:
        current_state = MirWindowState(mir_window_event_get_attribute_value(event));
        window->handle_resize(requested_size);
        break;

    default:;
    }

    seat->for_each_listener(client, [this, event](WlKeyboard* keyboard)
        {
            keyboard->handle_window_event(event, surface);
        });
}

