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
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::LegacySurfaceChangeNotification::LegacySurfaceChangeNotification(
    std::function<void()> const& notify_scene_change,
    std::function<void(int)> const& notify_buffer_change) :
    notify_scene_change(notify_scene_change),
    notify_buffer_change(notify_buffer_change)
{
}

void ms::LegacySurfaceChangeNotification::resized_to(geom::Size const& /*size*/)
{
    notify_scene_change();
}

void ms::LegacySurfaceChangeNotification::moved_to(geom::Point const& /*top_left*/)
{
    notify_scene_change();
}

void ms::LegacySurfaceChangeNotification::hidden_set_to(bool /*hide*/)
{
    notify_scene_change();
}

void ms::LegacySurfaceChangeNotification::frame_posted(int frames_available)
{
    notify_buffer_change(frames_available);
}

void ms::LegacySurfaceChangeNotification::alpha_set_to(float /*alpha*/)
{
    notify_scene_change();
}

// An orientation change alone is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::orientation_set_to(MirOrientation /*orientation*/)
{
}

void ms::LegacySurfaceChangeNotification::transformation_set_to(glm::mat4 const& /*t*/)
{
    notify_scene_change();
}

// An attrib change alone is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::attrib_changed(MirSurfaceAttrib /* attrib */, int /* value */)
{
}

// Cursor image change request is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::cursor_image_set_to(mg::CursorImage const& /* image */)
{
}

void ms::LegacySurfaceChangeNotification::reception_mode_set_to(mi::InputReceptionMode /*mode*/)
{
    notify_scene_change();
}

// A client close request is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::client_surface_close_requested()
{
}

// A keymap change is not enough to trigger recomposition
void ms::LegacySurfaceChangeNotification::keymap_changed(xkb_rule_names const&)
{
}

void ms::LegacySurfaceChangeNotification::renamed(char const*)
{
    notify_scene_change();
}
