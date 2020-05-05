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
    geometry::Point const& root_position)
{
    auto const target_surface = root_surface->subsurface_at(root_position).value_or(root_surface);
    auto const position_on_target = root_position - target_surface->total_offset();
    auto const serial = wl_display_next_serial(wl_client_get_display(client));

    focused_surface_for_ids[touch_id] = target_surface;

    send_down_event(
        serial,
        ms.count(),
        target_surface->raw_resource(),
        touch_id,
        position_on_target.x.as_int(),
        position_on_target.y.as_int());
    can_send_frame = true;
}

void mf::WlTouch::motion(
    std::chrono::milliseconds const& ms,
    int32_t touch_id,
    WlSurface* /* root_surface */,
    geometry::Point const& root_position)
{
    auto const target_surface = focused_surface_for_ids.find(touch_id);

    if (target_surface == focused_surface_for_ids.end())
    {
        log_warning("WlTouch::motion() called with invalid ID %d", touch_id);
        return;
    }

    auto const position_on_target = root_position - target_surface->second->total_offset();

    send_motion_event(
        ms.count(),
        touch_id,
        position_on_target.x.as_int(),
        position_on_target.y.as_int());
    can_send_frame = true;
}

void mf::WlTouch::up(std::chrono::milliseconds const& ms, int32_t touch_id)
{
    auto const serial = wl_display_next_serial(wl_client_get_display(client));

    if (focused_surface_for_ids.erase(touch_id))
    {
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
