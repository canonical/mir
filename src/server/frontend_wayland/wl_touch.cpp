/*
 * Copyright © Canonical Ltd.
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
 */

#include "wl_touch.h"

#include "wayland_utils.h"
#include "wl_surface.h"
#include "wl_seat.h"

#include "mir/executor.h"
#include "mir_toolkit/events/event.h"
#include "mir/events/touch_event.h"
#include "mir/log.h"
#include "mir/time/clock.h"
#include "mir/wayland/client.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace geom = mir::geometry;

mf::WlTouch::WlTouch(wl_resource* new_resource, std::shared_ptr<time::Clock> const& clock)
    : Touch(new_resource, Version<8>()),
      clock{clock}
{
}

mf::WlTouch::~WlTouch()
{
    for (auto const& touch : touch_id_to_surface)
    {
        if (touch.second.surface)
        {
            touch.second.surface.value().remove_destroy_listener(touch.second.destroy_listener_id);
        }
    }
}

void mf::WlTouch::event(std::shared_ptr<MirTouchEvent const> const& event, WlSurface& root_surface)
{
    std::chrono::milliseconds timestamp{mir_input_event_get_wayland_timestamp(mir_touch_event_input_event(event.get()))};

    for (auto i = 0u; i < mir_touch_event_point_count(event.get()); ++i)
    {
        if (!event->local_position(i))
        {
            continue;
        }
        auto const position = event->local_position(i).value();
        int const touch_id = mir_touch_event_id(event.get(), i);
        MirTouchAction const action = mir_touch_event_action(event.get(), i);

        switch (action)
        {
        case mir_touch_action_down:
        {
            auto const serial = client->next_serial(event);
            down(serial, timestamp, touch_id, root_surface, position);
        }   break;

        case mir_touch_action_up:
        {
            auto const serial = client->next_serial(event);
            up(serial, timestamp, touch_id);
        }   break;

        case mir_touch_action_change:
            motion(timestamp, touch_id, position);
            break;

        case mir_touch_actions:;
        }
    }

    maybe_frame();
}

void mf::WlTouch::down(
    uint32_t serial,
    std::chrono::milliseconds const& ms,
    int32_t touch_id,
    WlSurface& root_surface,
    geometry::PointF root_position)
{
    geom::Point root_point{root_position};
    auto const target_surface = root_surface.subsurface_at(root_point).value_or(&root_surface);
    auto const offset = target_surface->total_offset();
    auto const position_on_target = root_position - geom::DisplacementF{offset};

    auto const listener_id = target_surface->add_destroy_listener(
        [this, touch_id]()
        {
            auto const serial = client->next_serial(nullptr);
            auto const timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                clock->now().time_since_epoch());
            up(serial, timestamp, touch_id);
            maybe_frame();
        });
    touch_id_to_surface[touch_id] = {mw::make_weak(target_surface), listener_id};

    send_down_event(
        serial,
        ms.count(),
        target_surface->raw_resource(),
        touch_id,
        position_on_target.x.as_value(),
        position_on_target.y.as_value());
    needs_frame = true;
}

void mf::WlTouch::motion(
    std::chrono::milliseconds const& ms,
    int32_t touch_id,
    geometry::PointF root_position)
{
    auto const touch = touch_id_to_surface.find(touch_id);

    if (touch == touch_id_to_surface.end())
    {
        log_warning("WlTouch::motion(): invalid ID %d", touch_id);
        return;
    }
    else if (!touch->second.surface)
    {
        log_warning("WlTouch::motion(): ID %d maps to destroyed surface", touch_id);
        return;
    }

    auto const offset = touch->second.surface.value().total_offset();
    auto const position_on_target = root_position - geom::DisplacementF{offset};

    send_motion_event(
        ms.count(),
        touch_id,
        position_on_target.x.as_value(),
        position_on_target.y.as_value());
    needs_frame = true;
}

void mf::WlTouch::up(uint32_t serial, std::chrono::milliseconds const& ms, int32_t touch_id)
{
    auto const touch = touch_id_to_surface.find(touch_id);
    if (touch != touch_id_to_surface.end())
    {
        if (touch->second.surface)
        {
            touch->second.surface.value().remove_destroy_listener(touch->second.destroy_listener_id);
        }
        touch_id_to_surface.erase(touch);
        send_up_event(
            serial,
            ms.count(),
            touch_id);
        needs_frame = true;
    }
    else
    {
        log_warning("WlTouch::up() called with invalid ID %d", touch_id);
    }
}

void mf::WlTouch::maybe_frame()
{
    if (needs_frame)
    {
        send_frame_event();
        needs_frame = false;
    }
}
