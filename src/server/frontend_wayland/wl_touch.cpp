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
#include "mir/log.h"

namespace mf = mir::frontend;

mf::WlTouch::WlTouch(
    wl_resource* new_resource,
    std::function<void(WlTouch*)> const& on_destroy)
    : Touch(new_resource),
      on_destroy{on_destroy}
{
}

mf::WlTouch::~WlTouch()
{
    on_destroy(this);
}

void mf::WlTouch::handle_event(MirTouchEvent const* touch_ev, WlSurface* main_surface)
{
    // TODO: support for touches on subsurfaces
    auto const input_ev = mir_touch_event_input_event(touch_ev);
    auto const ev = mir::client::Event{mir_input_event_get_event(input_ev)};

    for (auto i = 0u; i < mir_touch_event_point_count(touch_ev); ++i)
    {
        auto const point = geometry::Point{mir_touch_event_axis_value(touch_ev, i, mir_touch_axis_x),
                                           mir_touch_event_axis_value(touch_ev, i, mir_touch_axis_y)};
        auto const touch_id = mir_touch_event_id(touch_ev, i);
        auto const action = mir_touch_event_action(touch_ev, i);

        switch (action)
        {
        case mir_touch_action_down:
        {
            auto const transformed = main_surface->transform_point(point);
            handle_down(transformed.position,
                        transformed.surface,
                        mir_input_event_get_event_time_ms(input_ev),
                        touch_id);
            break;
        }
        case mir_touch_action_up:
            handle_up(mir_input_event_get_event_time_ms(input_ev), touch_id);
            break;
        case mir_touch_action_change:
        {
            auto current_surface = focused_surface_for_ids.find(touch_id);
            if (current_surface == focused_surface_for_ids.end())
            {
                log_warning("WlTouch::handle_event() called with mir_touch_action_change action but touch id that has not been registered");
            }
            else
            {
                auto const transformed_point = current_surface->second->total_offset() + point;
                send_motion_event(mir_input_event_get_event_time_ms(input_ev),
                                  touch_id,
                                  transformed_point.x.as_int(),
                                  transformed_point.y.as_int());
            }
            break;
        }
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
        send_frame_event();
    }
}

void mf::WlTouch::release()
{
    destroy_wayland_object();
}

void mf::WlTouch::handle_down(mir::geometry::Point position, WlSurface* surface, uint32_t time, int32_t id)
{
    focused_surface_for_ids[id] = surface;
    // TODO: Why is this not wl_display_next_serial()?
    send_down_event(wl_display_get_serial(wl_client_get_display(client)),
                    time,
                    surface->raw_resource(),
                    id,
                    position.x.as_int(),
                    position.y.as_int());
}

void mf::WlTouch::handle_up(uint32_t time, int32_t id)
{
    focused_surface_for_ids.erase(id);
    // TODO: Why is this not wl_display_next_serial()?
    send_up_event(wl_display_get_serial(wl_client_get_display(client)),
                  time,
                  id);
}

