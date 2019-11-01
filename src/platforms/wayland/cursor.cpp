/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "cursor.h"

#include <wayland-client.h>

namespace mpw = mir::platform::wayland;

mpw::Cursor::Cursor(wl_display* display, std::shared_ptr<graphics::CursorImage> const& default_image) :
    display(display),
    default_image(default_image)
{
//    puts(__PRETTY_FUNCTION__);
}

mpw::Cursor::~Cursor() = default;

void mpw::Cursor::move_to(geometry::Point)
{
//    puts(__PRETTY_FUNCTION__);
}

void mpw::Cursor::show(graphics::CursorImage const& /*cursor_image*/)
{
//    puts(__PRETTY_FUNCTION__);
}

void mpw::Cursor::show()
{
//    puts(__PRETTY_FUNCTION__);
}

void mpw::Cursor::hide()
{
//    puts(__PRETTY_FUNCTION__);
}
