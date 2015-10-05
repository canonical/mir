/*
 * Copyright © 2015 Canonical Ltd.
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

#include "cursor.h"

#include "host_connection.h"

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::Cursor::Cursor(std::shared_ptr<HostConnection> const& host_connection, std::shared_ptr<mg::CursorImage> const& default_image)
    : connection(host_connection),
      default_image(default_image)
{
    connection->set_cursor_image(*default_image);
}

mgn::Cursor::~Cursor()
{
}

void mgn::Cursor::move_to(geom::Point)
{
}

void mgn::Cursor::show(mg::CursorImage const& cursor_image)
{
    connection->set_cursor_image(cursor_image);
}

void mgn::Cursor::show()
{
    connection->set_cursor_image(*default_image);
}

void mgn::Cursor::hide()
{
    connection->hide_cursor();
}
