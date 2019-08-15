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

#include "wl_surface_event_sink.h"
#include "wl_seat.h"
#include "wl_pointer.h"
#include "wl_keyboard.h"
#include "wl_touch.h"
#include "wl_surface.h"
#include "wayland_utils.h"
#include "window_wl_surface_role.h"

#include <mir_toolkit/events/window_placement.h>

#include <linux/input-event-codes.h>

namespace mf = mir::frontend;
namespace geom = mir::geometry;

mf::WlSurfaceEventSink::WlSurfaceEventSink(WlSeat* seat, wl_client* client, WlSurface* surface,
                                           WindowWlSurfaceRole* window)
    : seat{seat},
      client{client},
      surface{surface},
      window{window},
      window_size{geometry::Size{0,0}},
      destroyed{std::make_shared<bool>(false)}
{
}

mf::WlSurfaceEventSink::~WlSurfaceEventSink()
{
    *destroyed = true;
}

void mf::WlSurfaceEventSink::handle_event(EventUPtr&& event)
{
    seat->spawn(run_unless(
        destroyed,
        [this, event = std::shared_ptr<MirEvent>{move(event)}]()
        {
            switch (mir_event_get_type(event.get()))
            {
                case mir_event_type_resize:
                {
                    auto const resize_event{mir_event_get_resize_event(event.get())};
                    geometry::Size const new_size{mir_resize_event_get_width(resize_event),
                                                  mir_resize_event_get_height(resize_event)};
                    handle_resize(new_size);
                    break;
                }
                case mir_event_type_input:
                    handle_input_event(mir_event_get_input_event(event.get()));
                    break;
                case mir_event_type_keymap:
                    handle_keymap_event(mir_event_get_keymap_event(event.get()));
                    break;
                case mir_event_type_window:
                    handle_window_event(mir_event_get_window_event(event.get()));
                    break;
                case mir_event_type_window_placement:
                {
                    auto const placement_event{mir_event_get_window_placement_event(event.get())};
                    auto const rect = mir_window_placement_get_relative_position(placement_event);
                    window->handle_resize(geom::Point{rect.left, rect.top}, geom::Size{rect.width, rect.height});
                    break;
                }
                case mir_event_type_close_window:
                    window->handle_close_request();
                    break;
                default:
                    break;
            }
        }));
}

void mf::WlSurfaceEventSink::handle_resize(mir::geometry::Size const& new_size)
{
    if (new_size != window_size)
    {
        requested_size = new_size;
        window->handle_resize(std::experimental::nullopt, new_size);
    }
}

void mf::WlSurfaceEventSink::handle_input_event(MirInputEvent const* event)
{
    auto const ns = std::chrono::nanoseconds{mir_input_event_get_event_time(event)};
    auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);

    // Remember the timestamp of any events "signed" with a cookie
    if (mir_input_event_has_cookie(event))
        timestamp_ns = ns.count();

    switch (mir_input_event_get_type(event))
    {
    case mir_input_event_type_key:
    {
        MirKeyboardEvent const* keyboard_ev = mir_input_event_get_keyboard_event(event);
        MirKeyboardAction const action = mir_keyboard_event_action(keyboard_ev);
        if (action == mir_keyboard_action_down || action == mir_keyboard_action_up)
        {
            int const scancode = mir_keyboard_event_scan_code(keyboard_ev);
            bool const down = action == mir_keyboard_action_down;
            seat->for_each_listener(client, [&ms, scancode, down](WlKeyboard* keyboard)
                {
                    keyboard->key(ms, scancode, down);
                });
        }
        break;
    }
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

void mf::WlSurfaceEventSink::handle_keymap_event(MirKeymapEvent const* event)
{
    char const* buffer;
    size_t length;

    mir_keymap_event_get_keymap_buffer(event, &buffer, &length);

    seat->for_each_listener(client, [buffer, length](WlKeyboard* keyboard)
        {
            keyboard->set_keymap(buffer, length);
        });
}

void mf::WlSurfaceEventSink::handle_window_event(MirWindowEvent const* event)
{
    switch (mir_window_event_get_attribute(event))
    {
    case mir_window_attrib_focus:
        has_focus = mir_window_event_get_attribute_value(event);
        if (has_focus)
            seat->notify_focus(client);
        window->handle_active_change(has_focus);
        seat->for_each_listener(client, [surface = surface, has_focus = has_focus](WlKeyboard* keyboard)
            {
                keyboard->focussed(surface, has_focus);
            });
        break;

    case mir_window_attrib_state:
        current_state = MirWindowState(mir_window_event_get_attribute_value(event));
        window->handle_state_change(current_state);
        break;

    default:;
    }
}

