/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "surface_change_notification.h"

#include "mir/scene/scene_change_notification.h"
#include "mir/scene/surface.h"
#include "mir/geometry/displacement.h"

#include <boost/throw_exception.hpp>

namespace ms = mir::scene;
namespace geom = mir::geometry;

ms::SceneChangeNotification::SceneChangeNotification(
    std::function<void()> const& scene_notify_change,
    std::function<void(geom::Rectangle const& damage)> const& damage_notify_change) :
    scene_notify_change(scene_notify_change),
    damage_notify_change(damage_notify_change)
{
}

ms::SceneChangeNotification::~SceneChangeNotification()
{
    end_observation();
}

void ms::SceneChangeNotification::add_surface_observer(ms::Surface* surface)
{
    auto notifier = [surface, this, was_visible = false] () mutable
        {
            if (surface->visible() || was_visible)
                scene_notify_change();
            was_visible = surface->visible();
        };

    auto observer = std::make_shared<SurfaceChangeNotification>(surface, notifier, damage_notify_change);
    surface->register_interest(observer);

    std::unique_lock lg(surface_observers_guard);
    surface_observers[surface] = observer;
}

void ms::SceneChangeNotification::surface_added(std::shared_ptr<ms::Surface> const& surface)
{
    add_surface_observer(surface.get());

    // If the surface already has content we need to (re)composite
    if (surface->visible())
        scene_notify_change();
}

void ms::SceneChangeNotification::surface_exists(std::shared_ptr<ms::Surface> const& surface)
{
    add_surface_observer(surface.get());
}
    
void ms::SceneChangeNotification::surface_removed(std::shared_ptr<ms::Surface> const& surface)
{
    {
        std::unique_lock lg(surface_observers_guard);
        auto it = surface_observers.find(surface.get());
        if (it != surface_observers.end())
        {
            surface->unregister_interest(*it->second);
            surface_observers.erase(it);
        }
    }

    if (surface->visible())
        scene_notify_change();
}

void ms::SceneChangeNotification::surfaces_reordered(SurfaceSet const&)
{
    scene_notify_change();
}

void ms::SceneChangeNotification::scene_changed()
{
    scene_notify_change();
}

void ms::SceneChangeNotification::end_observation()
{
    std::unique_lock lg(surface_observers_guard);
    for (auto &kv : surface_observers)
    {
        auto surface = kv.first;
        if (surface)
        {
            surface->unregister_interest(*kv.second);
        }
    }
    surface_observers.clear();
}
