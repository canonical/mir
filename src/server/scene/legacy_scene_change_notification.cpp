/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "legacy_surface_change_notification.h"

#include "mir/scene/legacy_scene_change_notification.h"
#include "mir/scene/surface.h"

#include <boost/throw_exception.hpp>

namespace ms = mir::scene;

ms::LegacySceneChangeNotification::LegacySceneChangeNotification(
    std::function<void()> const& scene_notify_change,
    std::function<void(int)> const& buffer_notify_change)
    : scene_notify_change(scene_notify_change),
      buffer_notify_change(buffer_notify_change)
      
{
}

ms::LegacySceneChangeNotification::LegacySceneChangeNotification(
    std::function<void()> const& scene_notify_change,
    std::function<void(int frames, mir::geometry::Rectangle const& damage)> const& damage_notify_change) :
    scene_notify_change(scene_notify_change),
    damage_notify_change(damage_notify_change)
{
}

ms::LegacySceneChangeNotification::~LegacySceneChangeNotification()
{
    end_observation();
}

namespace
{
class NonLegacySurfaceChangeNotification : public ms::LegacySurfaceChangeNotification
{
public:
    NonLegacySurfaceChangeNotification(
        std::function<void()> const& notify_scene_change,
        std::function<void(int frames, mir::geometry::Rectangle const& damage)> const& damage_notify_change,
        ms::Surface* surface);

    void moved_to(ms::Surface const* surf, const mir::geometry::Point&) override;
    void frame_posted(ms::Surface const* surf, int frames_available, const mir::geometry::Size& size) override;

private:
    mir::geometry::Point top_left;
    std::function<void(int frames, mir::geometry::Rectangle const& damage)> const damage_notify_change;
};

NonLegacySurfaceChangeNotification::NonLegacySurfaceChangeNotification(
    std::function<void()> const& notify_scene_change,
    std::function<void(int frames, mir::geometry::Rectangle const& damage)> const& damage_notify_change,
    ms::Surface* surface) :
    ms::LegacySurfaceChangeNotification(notify_scene_change, {}),
    damage_notify_change(damage_notify_change)
{
    top_left = surface->top_left();
}

void NonLegacySurfaceChangeNotification::moved_to(ms::Surface const* surf, const mir::geometry::Point& top_left)
{
    this->top_left = top_left;
    ms::LegacySurfaceChangeNotification::moved_to(surf, top_left);
}

void NonLegacySurfaceChangeNotification::frame_posted(ms::Surface const*, int frames_available, const mir::geometry::Size& size)
{
    mir::geometry::Rectangle const update_region{top_left, size};
    damage_notify_change(frames_available, update_region);
}
}

void ms::LegacySceneChangeNotification::add_surface_observer(ms::Surface* surface)
{
    auto notifier = [surface, this, was_visible = false] () mutable
        {
            if (surface->visible() || was_visible)
                scene_notify_change();
            was_visible = surface->visible();
        };

    if (buffer_notify_change)
    {
        auto observer = std::make_shared<LegacySurfaceChangeNotification>(notifier, buffer_notify_change);
        surface->add_observer(observer);

        std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
        surface_observers[surface] = observer;
    }
    else
    {
        auto observer = std::make_shared<NonLegacySurfaceChangeNotification>(notifier, damage_notify_change, surface);
        surface->add_observer(observer);

        std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
        surface_observers[surface] = observer;
    }
}

void ms::LegacySceneChangeNotification::surface_added(std::shared_ptr<ms::Surface> const& surface)
{
    add_surface_observer(surface.get());

    // If the surface already has content we need to (re)composite
    if (!buffer_notify_change && surface->visible())
        scene_notify_change();
}

void ms::LegacySceneChangeNotification::surface_exists(std::shared_ptr<ms::Surface> const& surface)
{
    add_surface_observer(surface.get());
}
    
void ms::LegacySceneChangeNotification::surface_removed(std::shared_ptr<ms::Surface> const& surface)
{
    {
        std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
        auto it = surface_observers.find(surface.get());
        if (it != surface_observers.end())
        {
            surface->remove_observer(it->second);
            surface_observers.erase(it);
        }
    }

    if (surface->visible())
        scene_notify_change();
}

void ms::LegacySceneChangeNotification::surfaces_reordered()
{
    scene_notify_change();
}

void ms::LegacySceneChangeNotification::scene_changed()
{
    scene_notify_change();
}

void ms::LegacySceneChangeNotification::end_observation()
{
    std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
    for (auto &kv : surface_observers)
    {
        auto surface = kv.first;
        if (surface)
            surface->remove_observer(kv.second);
    }
    surface_observers.clear();
}
