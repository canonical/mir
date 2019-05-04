/*
 * Copyright © 2014 Canonical Ltd.
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

void ms::LegacySurfaceChangeNotification::resized_to(Surface const*, geometry::Size const&)
{
    notify_scene_change();
}

void ms::LegacySurfaceChangeNotification::moved_to(Surface const*, geometry::Point const&)
{
    notify_scene_change();
}

void ms::LegacySurfaceChangeNotification::hidden_set_to(Surface const*, bool)
{
    notify_scene_change();
}

void ms::LegacySurfaceChangeNotification::frame_posted(Surface const*, int frames_available, geometry::Size const&)
{
    notify_buffer_change(frames_available);
}

void ms::LegacySurfaceChangeNotification::alpha_set_to(Surface const*, float)
{
    notify_scene_change();
}

// An orientation change alone is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::orientation_set_to(Surface const*, MirOrientation)
{
}

void ms::LegacySurfaceChangeNotification::transformation_set_to(Surface const*, glm::mat4 const&)
{
    notify_scene_change();
}

// An attrib change alone is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::attrib_changed(Surface const*, MirWindowAttrib, int)
{
}

// Cursor image change request is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::cursor_image_set_to(Surface const*, graphics::CursorImage const&)
{
}

void ms::LegacySurfaceChangeNotification::cursor_image_removed(Surface const*)
{
}

void ms::LegacySurfaceChangeNotification::placed_relative(Surface const*, geometry::Rectangle const&)
{
}

void ms::LegacySurfaceChangeNotification::reception_mode_set_to(Surface const*, input::InputReceptionMode)
{
    notify_scene_change();
}

// A client close request is not enough to trigger recomposition.
void ms::LegacySurfaceChangeNotification::client_surface_close_requested(Surface const*)
{
}

// A keymap change is not enough to trigger recomposition
void ms::LegacySurfaceChangeNotification::keymap_changed(Surface const*, MirInputDeviceId, std::string const&,
                                                         std::string const&, std::string const&,
                                                         std::string const&)
{
}

void ms::LegacySurfaceChangeNotification::renamed(Surface const*, char const*)
{
    notify_scene_change();
}

void ms::LegacySurfaceChangeNotification::input_consumed(Surface const*, MirEvent const*)
{
}

void ms::LegacySurfaceChangeNotification::start_drag_and_drop(Surface const*, std::vector<uint8_t> const&)
{
}

void ms::LegacySurfaceChangeNotification::z_index_set_to(Surface const*, int)
{
}
