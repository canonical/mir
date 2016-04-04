/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/events/resize_event.h"

MirResizeEvent::MirResizeEvent() :
    MirEvent(mir_event_type_resize)
{
}

int MirResizeEvent::surface_id() const
{
    return surface_id_;
}

void MirResizeEvent::set_surface_id(int id)
{
    surface_id_ = id;
}

int MirResizeEvent::width() const
{
    return width_;
}

void MirResizeEvent::set_width(int width)
{
    width_ = width;
}

int MirResizeEvent::height() const
{
    return height_;
}

void MirResizeEvent::set_height(int height)
{
    height_ = height;
}
