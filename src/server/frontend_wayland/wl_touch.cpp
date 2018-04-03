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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wl_touch.h"

#include "wayland_utils.h"
#include "wl_surface.h"

#include "mir/executor.h"
#include "mir/client/event.h"

namespace mf = mir::frontend;

mf::WlTouch::WlTouch(
    wl_client* client,
    wl_resource* parent,
    uint32_t id,
    std::function<void(WlTouch*)> const& on_destroy)
    : Touch(client, parent, id),
      on_destroy{on_destroy}
{
}

mf::WlTouch::~WlTouch()
{
    on_destroy(this);
}

void mf::WlTouch::handle_event(MirTouchEvent const* touch_ev, WlSurface* surface)
{
    // TODO: support for touches on subsurfaces
    auto const input_ev = mir_touch_event_input_event(touch_ev);
    auto const ev = mir::client::Event{mir_input_event_get_event(input_ev)};
    auto const buffer_offset = surface->buffer_offset();

    for (auto i = 0u; i < mir_touch_event_point_count(touch_ev); ++i)
    {
        auto const touch_id = mir_touch_event_id(touch_ev, i);
        auto const action = mir_touch_event_action(touch_ev, i);
        auto const x = mir_touch_event_axis_value(
            touch_ev,
            i,
            mir_touch_axis_x) - buffer_offset.dx.as_int();
        auto const y = mir_touch_event_axis_value(
            touch_ev,
            i,
            mir_touch_axis_y) - buffer_offset.dy.as_int();

        switch (action)
        {
        case mir_touch_action_down:
            wl_touch_send_down(
                resource,
                wl_display_get_serial(wl_client_get_display(client)),
                mir_input_event_get_event_time_ms(input_ev),
                surface->raw_resource(),
                touch_id,
                wl_fixed_from_double(x),
                wl_fixed_from_double(y));
            break;
        case mir_touch_action_up:
            wl_touch_send_up(
                resource,
                wl_display_get_serial(wl_client_get_display(client)),
                mir_input_event_get_event_time_ms(input_ev),
                touch_id);
            break;
        case mir_touch_action_change:
            wl_touch_send_motion(
                resource,
                mir_input_event_get_event_time_ms(input_ev),
                touch_id,
                wl_fixed_from_double(x),
                wl_fixed_from_double(y));
            break;
        case mir_touch_actions:
            /*
                * We should never receive an event with this action set;
                * the only way would be if a *new* action has been added
                * to the enum, and this hasn't been updated.
                *
                * There's nothing to do here, but don't use default: so
                * that the compiler will warn if a new enum value is added.
                */
            break;
        }
    }

    if (mir_touch_event_point_count(touch_ev) > 0)
    {
        /*
            * This is mostly paranoia; I assume we won't actually be called
            * with an empty touch event.
            *
            * Regardless, the Wayland protocol requires that there be at least
            * one event sent before we send the ending frame, so make that explicit.
            */
        wl_touch_send_frame(resource);
    }
}

void mf::WlTouch::release()
{
    wl_resource_destroy(resource);
}

