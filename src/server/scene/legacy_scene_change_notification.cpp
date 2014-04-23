/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "legacy_surface_change_notification.h"

#include "mir/scene/legacy_scene_change_notification.h"
#include "mir/scene/surface.h"

#include <boost/throw_exception.hpp>

// TODO: Place under test

namespace ms = mir::scene;

ms::LegacySceneChangeNotification::LegacySceneChangeNotification(std::function<void()> const& notify_change)
    : notify_change(notify_change)
{
}

ms::LegacySceneChangeNotification::~LegacySceneChangeNotification()
{
}

void ms::LegacySceneChangeNotification::surface_added(std::shared_ptr<ms::Surface> const& surface)
{
    auto observer = std::make_shared<ms::LegacySurfaceChangeNotification>(notify_change);
    surface->add_observer(observer);
    
    {
        std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
        surface_observers[surface] = observer;
        // TODO: Throw exception on duplicate
    }
    notify_change();
}
    
void ms::LegacySceneChangeNotification::surface_removed(std::shared_ptr<ms::Surface> const& surface)
{
    {
        std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
        auto it = surface_observers.find(surface);
        if (it != surface_observers.end())
            surface_observers.erase(it);
        // TODO: Throw exception
    }
    notify_change();
}

void ms::LegacySceneChangeNotification::surfaces_reordered()
{
    notify_change();
}

