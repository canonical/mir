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
        handle_keyboard_event(ms, mir_input_event_get_keyboard_event(event));
        break;
    case mir_input_event_type_pointer:
        handle_pointer_event(ms, mir_input_event_get_pointer_event(event));
        break;
    case mir_input_event_type_touch:
        handle_touch_event(ms, mir_input_event_get_touch_event(event));
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

void mf::WlSurfaceEventSink::handle_keyboard_event(std::chrono::milliseconds const& ms, MirKeyboardEvent const* event)
{
    MirKeyboardAction const action = mir_keyboard_event_action(event);
    if (action == mir_keyboard_action_down || action == mir_keyboard_action_up)
    {
        int const scancode = mir_keyboard_event_scan_code(event);
        bool const down = action == mir_keyboard_action_down;
        seat->for_each_listener(client, [&ms, scancode, down](WlKeyboard* keyboard)
            {
                keyboard->key(ms, scancode, down);
            });
    }
}

void mf::WlSurfaceEventSink::handle_pointer_event(std::chrono::milliseconds const& ms, MirPointerEvent const* event)
{
    switch(mir_pointer_event_action(event))
    {
        case mir_pointer_action_button_down:
        case mir_pointer_action_button_up:
            handle_pointer_button_event(ms, event);
            break;
        case mir_pointer_action_enter:
        {
            geom::Point const position{
                mir_pointer_event_axis_value(event, mir_pointer_axis_x),
                mir_pointer_event_axis_value(event, mir_pointer_axis_y)};
            seat->for_each_listener(client, [surface = surface, &position](WlPointer* pointer)
                {
                    pointer->enter(surface, position);
                    pointer->frame();
                });
            break;
        }
        case mir_pointer_action_leave:
            seat->for_each_listener(client, [](WlPointer* pointer)
                {
                    pointer->leave();
                    pointer->frame();
                });
            break;
        case mir_pointer_action_motion:
            handle_pointer_motion_event(ms, event);
            break;
        case mir_pointer_actions:
            break;
    }
}

void mf::WlSurfaceEventSink::handle_pointer_button_event(
    std::chrono::milliseconds const& ms,
    MirPointerEvent const* event)
{
    MirPointerButtons const event_buttons = mir_pointer_event_buttons(event);
    std::vector<std::pair<uint32_t, bool>> buttons;

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
        if (mapping.first & (event_buttons ^ last_pointer_buttons))
        {
            bool const pressed = (mapping.first & event_buttons);
            buttons.push_back(std::make_pair(mapping.second, pressed));
        }
    }

    if (!buttons.empty())
    {
        seat->for_each_listener(client, [&ms, &buttons](WlPointer* pointer)
            {
                for (auto& button : buttons)
                {
                    pointer->button(ms, button.first, button.second);
                }
                pointer->frame();
            });
    }

    last_pointer_buttons = event_buttons;
}

void mf::WlSurfaceEventSink::handle_pointer_motion_event(
    std::chrono::milliseconds const& ms,
    MirPointerEvent const* event)
{
    // TODO: send axis_source, axis_stop and axis_discrete events where appropriate
    // (may require significant eworking of the input system)

    geom::Point const position{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};
    geom::Displacement const axis_motion{
        mir_pointer_event_axis_value(event, mir_pointer_axis_hscroll) * 10,
        mir_pointer_event_axis_value(event, mir_pointer_axis_vscroll) * 10};
    bool const send_motion = (!last_pointer_position || position != last_pointer_position.value());
    bool const send_axis = (axis_motion != geom::Displacement{});

    last_pointer_position = position;

    if (send_motion || send_axis)
    {
        seat->for_each_listener(
            client,
            [&ms, surface = surface, &send_motion, &position, &send_axis, &axis_motion](WlPointer* pointer)
            {
                if (send_motion)
                    pointer->motion(ms, surface, position);
                if (send_axis)
                    pointer->axis(ms, axis_motion);
                pointer->frame();
            });
    }
}

void mf::WlSurfaceEventSink::handle_touch_event(
    std::chrono::milliseconds const& ms,
    MirTouchEvent const* event)
{
    for (auto i = 0u; i < mir_touch_event_point_count(event); ++i)
    {
        geometry::Point const position{
            mir_touch_event_axis_value(event, i, mir_touch_axis_x),
            mir_touch_event_axis_value(event, i, mir_touch_axis_y)};
        int const touch_id = mir_touch_event_id(event, i);
        MirTouchAction const action = mir_touch_event_action(event, i);

        switch (action)
        {
        case mir_touch_action_down:
            seat->for_each_listener(client, [&ms, touch_id, surface = surface, &position](WlTouch* touch)
                {
                    touch->down(ms, touch_id, surface, position);
                });
            break;
        case mir_touch_action_up:
            seat->for_each_listener(client, [&ms, touch_id](WlTouch* touch)
                {
                    touch->up(ms, touch_id);
                });
            break;
        case mir_touch_action_change:
            seat->for_each_listener(client, [&ms, touch_id, surface = surface, &position](WlTouch* touch)
                {
                    touch->motion(ms, touch_id, surface, position);
                });
            break;
        case mir_touch_actions:;
        }
    }

    seat->for_each_listener(client, [](WlTouch* touch)
        {
            touch->frame();
        });
}
