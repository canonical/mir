/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/geometry/rectangle.h"

#include <cassert>

namespace geom = mir::geometry;

geom::Point geom::Rectangle::bottom_right() const
{
    return {top_left.x.as_int() + size.width.as_int(),
            top_left.y.as_int() + size.height.as_int()};
}

bool geom::Rectangle::contains(Point const& p) const
{
    if (size.width == geom::Width{0} || size.height == geom::Height{0})
        return false;

    auto br = bottom_right();
    return p.x >= top_left.x && p.x < br.x &&
           p.y >= top_left.y && p.y < br.y;
}
