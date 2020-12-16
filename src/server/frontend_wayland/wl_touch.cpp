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

#include <chrono>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace geom = mir::geometry;

mf::WlTouch::WlTouch(
    wl_resource* new_resource,
    std::function<void(WlTouch*)> const& on_destroy)
    : Touch(new_resource, Version<6>()),
      on_destroy{on_destroy}
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
    on_destroy(this);
}

void mf::WlTouch::release()
{
    destroy_wayland_object();
}

void mf::WlTouch::down(
    std::chrono::milliseconds const& ms,
    int32_t touch_id,
    WlSurface* root_surface,
    std::pair<float, float> const& root_position)
{
    geom::Point root_point{root_position.first, root_position.second};
    auto const target_surface = root_surface->subsurface_at(root_point).value_or(root_surface);
    auto const offset = target_surface->total_offset();
    auto const position_on_target = std::make_pair(
        root_position.first - offset.dx.as_int(),
        root_position.second - offset.dy.as_int());
    auto const serial = wl_display_next_serial(wl_client_get_display(client));

    // We wont have a "real" timestamp from libinput, so we have to make our own based on the time offset
    auto const time_offset = ms - std::chrono::steady_clock::now().time_since_epoch();
    auto const listener_id = target_surface->add_destroy_listener(
        [this, touch_id, time_offset]()
        {
            auto const timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                time_offset + std::chrono::steady_clock::now().time_since_epoch());
            up(timestamp, touch_id);
            frame();
        });
    touch_id_to_surface[touch_id] = {mw::make_weak(target_surface), listener_id};

    send_down_event(
        serial,
        ms.count(),
        target_surface->raw_resource(),
        touch_id,
        position_on_target.first,
        position_on_target.second);
    can_send_frame = true;
}

void mf::WlTouch::motion(
    std::chrono::milliseconds const& ms,
    int32_t touch_id,
    WlSurface* /* root_surface */,
    std::pair<float, float> const& root_position)
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
    auto const position_on_target = std::make_pair(
        root_position.first - offset.dx.as_int(),
        root_position.second - offset.dy.as_int());

    send_motion_event(
        ms.count(),
        touch_id,
        position_on_target.first,
        position_on_target.second);
    can_send_frame = true;
}

void mf::WlTouch::up(std::chrono::milliseconds const& ms, int32_t touch_id)
{
    auto const serial = wl_display_next_serial(wl_client_get_display(client));

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
        can_send_frame = true;
    }
    else
    {
        log_warning("WlTouch::up() called with invalid ID %d", touch_id);
    }
}

void mf::WlTouch::frame()
{
    if (can_send_frame)
        send_frame_event();
    can_send_frame = false;
}
