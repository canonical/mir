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
        send_resize(requested_size);
}

void mf::BasicSurfaceEventSink::handle_input_event(MirInputEvent const* event)
{
    // Remember the timestamp of any events "signed" with a cookie
    if (mir_input_event_has_cookie(event))
        timestamp_ns = mir_input_event_get_event_time(event);

    switch (mir_input_event_get_type(event))
    {
    case mir_input_event_type_key:
        seat->for_each_listener(client, [this, event](WlKeyboard* keyboard)
            {
                keyboard->handle_event(event, target);
            });
        break;
    case mir_input_event_type_pointer:
        handle_pointer_event(mir_input_event_get_pointer_event(event));
        break;
    case mir_input_event_type_touch:
        seat->for_each_listener(client, [this, event](WlTouch* touch)
            {
                touch->handle_event(event, target);
            });
        break;
    default:
        break;
    }
}

void mf::BasicSurfaceEventSink::handle_pointer_event(const MirPointerEvent* event)
{
    switch(mir_pointer_event_action(event))
    {
        case mir_pointer_action_button_down:
        case mir_pointer_action_button_up:
        {
            auto const current_pointer_buttons  = mir_pointer_event_buttons(event);
            auto const timestamp = mir_input_event_get_event_time_ms(mir_pointer_event_input_event(event));

            for (auto const& mapping :
                {
                    std::make_pair(mir_pointer_button_primary, BTN_LEFT),
                    std::make_pair(mir_pointer_button_secondary, BTN_RIGHT),
                    std::make_pair(mir_pointer_button_tertiary, BTN_MIDDLE),
                    std::make_pair(mir_pointer_button_back, BTN_BACK),
                    std::make_pair(mir_pointer_button_forward, BTN_FORWARD),
                    std::make_pair(mir_pointer_button_side, BTN_SIDE),
                    std::make_pair(mir_pointer_button_task, BTN_TASK),
                    std::make_pair(mir_pointer_button_extra, BTN_EXTRA)
                })
            {
                if (mapping.first & (current_pointer_buttons ^ last_pointer_buttons))
                {
                    auto const state = (mapping.first & current_pointer_buttons) ?
                        WL_POINTER_BUTTON_STATE_PRESSED :
                        WL_POINTER_BUTTON_STATE_RELEASED;

                    seat->for_each_listener(client, [&](WlPointer* pointer)
                        {
                            pointer->handle_button(timestamp, mapping.second, state);
                        });
                }
            }

            last_pointer_buttons = current_pointer_buttons;
            break;
        }
        case mir_pointer_action_enter:
        {
            seat->for_each_listener(client, [&](WlPointer* pointer)
                {
                    auto point = geom::Point{mir_pointer_event_axis_value(event, mir_pointer_axis_x),
                                             mir_pointer_event_axis_value(event, mir_pointer_axis_y)};
                    //auto transformed = WlSurface::from(target)->transform_point(point);
                    pointer->handle_enter(point - WlSurface::from(target)->buffer_offset(), target);
                });
            break;
        }
        case mir_pointer_action_leave:
        {
            seat->for_each_listener(client, [&](WlPointer* pointer)
                {
                    pointer->handle_leave(target);
                });
            break;
        }
        case mir_pointer_action_motion:
        {
            // TODO: properly group vscroll and hscroll events in the same frame (as described by the frame
            //  event description in wayland.xml) and send axis_source, axis_stop and axis_discrete events where
            //  appropriate (may require significant reworking of the input system)

            auto const timestamp = mir_input_event_get_event_time_ms(mir_pointer_event_input_event(event));

            seat->for_each_listener(client, [&](WlPointer* pointer)
                {
                    auto point = geom::Point{mir_pointer_event_axis_value(event, mir_pointer_axis_x),
                                             mir_pointer_event_axis_value(event, mir_pointer_axis_y)};
                    //auto transformed = WlSurface::from(target)->transform_point(point);
                    pointer->handle_motion(timestamp, point - WlSurface::from(target)->buffer_offset());

                    auto hscroll = mir_pointer_event_axis_value(event, mir_pointer_axis_hscroll) * 10;
                    pointer->handle_axis(timestamp, WL_POINTER_AXIS_HORIZONTAL_SCROLL, hscroll);

                    auto vscroll = mir_pointer_event_axis_value(event, mir_pointer_axis_vscroll) * 10;
                    pointer->handle_axis(timestamp, WL_POINTER_AXIS_VERTICAL_SCROLL, vscroll);
                });
        }
        case mir_pointer_actions:
            break;
    }
}

void mf::BasicSurfaceEventSink::handle_keymap_event(MirKeymapEvent const* event)
{
    seat->for_each_listener(client, [this, event](WlKeyboard* keyboard)
        {
            keyboard->handle_event(event, target);
        });
}

void mf::BasicSurfaceEventSink::handle_window_event(MirWindowEvent const* event)
{
    switch (mir_window_event_get_attribute(event))
    {
    case mir_window_attrib_focus:
        has_focus = mir_window_event_get_attribute_value(event);
        send_resize(requested_size);
        break;

    case mir_window_attrib_state:
        current_state = MirWindowState(mir_window_event_get_attribute_value(event));
        send_resize(requested_size);
        break;

    default:;
    }

    seat->for_each_listener(client, [this, event](WlKeyboard* keyboard)
        {
            keyboard->handle_event(event, target);
        });
}

