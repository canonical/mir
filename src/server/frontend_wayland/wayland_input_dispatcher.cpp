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
namespace mw = mir::wayland;

mf::WaylandInputDispatcher::WaylandInputDispatcher(
    WlSeat* seat,
    WlSurface* wl_surface)
    : seat{seat},
      client{wl_surface->client},
      wl_surface{mw::make_weak(wl_surface)}
{
}

void mf::WaylandInputDispatcher::set_keymap(mi::Keymap const& keymap)
{
    if (!wl_surface)
    {
        return;
    }

    seat->for_each_listener(client, [&](WlKeyboard* keyboard)
        {
            keyboard->set_keymap(keymap);
        });
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
            keyboard->focussed(&wl_surface.value(), has_focus);
        });
}

void mf::WaylandInputDispatcher::handle_event(MirInputEvent const* event)
{
    if (!wl_surface)
    {
        return;
    }

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

void mf::WaylandInputDispatcher::handle_keyboard_event(std::chrono::milliseconds const& ms, MirKeyboardEvent const* event)
{
    if (!wl_surface)
    {
        fatal_error("wl_surface should have already been checked");
    }

    MirKeyboardAction const action = mir_keyboard_event_action(event);
    if (action == mir_keyboard_action_down || action == mir_keyboard_action_up)
    {
        int const scancode = mir_keyboard_event_scan_code(event);
        bool const down = action == mir_keyboard_action_down;
        seat->for_each_listener(client, [&](WlKeyboard* keyboard)
            {
                keyboard->key(ms, &wl_surface.value(), scancode, down);
            });
    }
}
