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

#include <mir/input/xkb_mapper.h>
#include <mir/input/keymap.h>
#include <mir/log.h>

#include <linux/input-event-codes.h>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mi = mir::input;

mf::WaylandInputDispatcher::WaylandInputDispatcher(
    WlSeat* seat,
    WlSurface* wl_surface)
    : seat{seat},
      client{wl_surface->client},
      wl_surface{wl_surface},
      wl_surface_destroyed{wl_surface->destroyed_flag()}
{
}

void mf::WaylandInputDispatcher::set_keymap(mi::Keymap const& keymap)
{
    if (*wl_surface_destroyed)
        return;

    seat->for_each_listener(client, [keymap](WlKeyboard* keyboard)
        {
            keyboard->set_keymap(keymap);
        });
}

void mf::WaylandInputDispatcher::set_focus(bool has_focus)
{
    if (*wl_surface_destroyed)
        return;

    if (has_focus)
        seat->notify_focus(client);

    seat->for_each_listener(client, [wl_surface = wl_surface, has_focus = has_focus](WlKeyboard* keyboard)
        {
            keyboard->focussed(wl_surface, has_focus);
        });
}

void mf::WaylandInputDispatcher::handle_event(MirEvent const* event)
{
    if (*wl_surface_destroyed)
        return;

    if (mir_event_get_type(event) != mir_event_type_input)
    {
        log_warning(
            "WaylandInputDispatcher::input_consumed() got non-input event type %d",
            mir_event_get_type(event));
        return;
    }

    auto const input_ev = mir_event_get_input_event(event);
    handle_input_event(input_ev);
}

void mf::WaylandInputDispatcher::handle_input_event(MirInputEvent const* event)
{
    auto const ns = std::chrono::nanoseconds{mir_input_event_get_event_time(event)};
    auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);

    // Remember the timestamp of any events "signed" with a cookie
    if (mir_input_event_has_cookie(event))
        timestamp = ns;

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

void mf::WaylandInputDispatcher::handle_keyboard_event(std::chrono::milliseconds const& ms, MirKeyboardEvent const* event)
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

void mf::WaylandInputDispatcher::handle_pointer_event(std::chrono::milliseconds const& ms, MirPointerEvent const* event)
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
            seat->for_each_listener(client, [wl_surface = wl_surface, &position](WlPointer* pointer)
                {
                    pointer->enter(wl_surface, position);
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

void mf::WaylandInputDispatcher::handle_pointer_button_event(
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

void mf::WaylandInputDispatcher::handle_pointer_motion_event(
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
            [&ms, wl_surface = wl_surface, &send_motion, &position, &send_axis, &axis_motion](WlPointer* pointer)
            {
                if (send_motion)
                    pointer->motion(ms, wl_surface, position);
                if (send_axis)
                    pointer->axis(ms, axis_motion);
                pointer->frame();
            });
    }
}

void mf::WaylandInputDispatcher::handle_touch_event(
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
            seat->for_each_listener(client, [&ms, touch_id, wl_surface = wl_surface, &position](WlTouch* touch)
                {
                    touch->down(ms, touch_id, wl_surface, position);
                });
            break;
        case mir_touch_action_up:
            seat->for_each_listener(client, [&ms, touch_id](WlTouch* touch)
                {
                    touch->up(ms, touch_id);
                });
            break;
        case mir_touch_action_change:
            seat->for_each_listener(client, [&ms, touch_id, wl_surface = wl_surface, &position](WlTouch* touch)
                {
                    touch->motion(ms, touch_id, wl_surface, position);
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
