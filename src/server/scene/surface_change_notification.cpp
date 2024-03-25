/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "surface_change_notification.h"

#include "mir/scene/surface.h"

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::SurfaceChangeNotification::SurfaceChangeNotification(
    ms::Surface* surface,
    std::function<void()> const& notify_scene_change,
    std::function<void(int, geom::Rectangle const&)> const& notify_buffer_change) :
    notify_scene_change(notify_scene_change),
    notify_buffer_change(notify_buffer_change)
{
    top_left = surface->top_left();
}

void ms::SurfaceChangeNotification::content_resized_to(Surface const*, geometry::Size const&)
{
    notify_scene_change();
}

void ms::SurfaceChangeNotification::moved_to(Surface const*, geometry::Point const& new_top_left)
{
    {
        std::lock_guard lock{mutex};
        top_left = new_top_left;
    }
    notify_scene_change();
}

void ms::SurfaceChangeNotification::hidden_set_to(Surface const*, bool)
{
    notify_scene_change();
}

void ms::SurfaceChangeNotification::frame_posted(
    Surface const*,
    geometry::Rectangle const& damage)
{
    std::unique_lock lock{mutex};
    geom::Rectangle global_damage{top_left + as_displacement(damage.top_left), damage.size};
    lock.unlock();
    notify_buffer_change(1, global_damage);
}

void ms::SurfaceChangeNotification::alpha_set_to(Surface const*, float)
{
    notify_scene_change();
}

void ms::SurfaceChangeNotification::transformation_set_to(Surface const*, glm::mat4 const&)
{
    notify_scene_change();
}

void ms::SurfaceChangeNotification::reception_mode_set_to(Surface const*, input::InputReceptionMode)
{
    notify_scene_change();
}

void ms::SurfaceChangeNotification::renamed(Surface const*, std::string const&)
{
    notify_scene_change();
}
