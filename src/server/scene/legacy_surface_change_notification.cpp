/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "legacy_surface_change_notification.h"

namespace ms = mir::scene;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::LegacySurfaceChangeNotification::LegacySurfaceChangeNotification(std::function<void()> const& notify_change) :
        notify_change(notify_change) {}

void ms::LegacySurfaceChangeNotification::resized_to(geom::Size const& /*size*/)
{
    notify_change();
}

void ms::LegacySurfaceChangeNotification::moved_to(geom::Point const& /*top_left*/)
{
    notify_change();
}

void ms::LegacySurfaceChangeNotification::hidden_set_to(bool /*hide*/)
{
    notify_change();
}

void ms::LegacySurfaceChangeNotification::frame_posted()
{
    notify_change();
}

void ms::LegacySurfaceChangeNotification::alpha_set_to(float /*alpha*/)
{
    notify_change();
}

void ms::LegacySurfaceChangeNotification::transformation_set_to(glm::mat4 const& /*t*/)
{
    notify_change();
}

// An attrib change alone is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::attrib_changed(MirSurfaceAttrib /* attrib */, int /* value */)
{
}

void ms::LegacySurfaceChangeNotification::reception_mode_set_to(mi::InputReceptionMode /*mode*/)
{
    notify_change();
}
