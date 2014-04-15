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

#include "mir/scene/simple_observer.h"
#include "mir/scene/surface.h"

namespace ms = mir::scene;

ms::SimpleObserver::SimpleObserver(std::function<void()> const& notify_change)
    : notify_change(notify_change)
{
}

void ms::SimpleObserver::surface_added(std::shared_ptr<ms::Surface> const& surface)
{
    surface->add_observer(std::make_shared<ms::LegacySurfaceChangeNotification>(notify_change));
    notify_change();
}
    
void ms::SimpleObserver::surface_removed(std::shared_ptr<ms::Surface> const& /* surface */)
{
    notify_change();
}

void ms::SimpleObserver::surfaces_reordered()
{
    notify_change();
}

