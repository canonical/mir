/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include "mir/geometry/displacement.h"

#include <algorithm>

namespace geom = mir::geometry;

geom::Point geom::Rectangle::bottom_right() const
{
    return top_left + as_displacement(size);
}

geom::Point geom::Rectangle::top_right() const
{
    return top_left + DeltaX{size.width.as_int()};
}

geom::Point geom::Rectangle::bottom_left() const
{
    return top_left + DeltaY{size.height.as_int()};
}

bool geom::Rectangle::contains(Rectangle const& r) const
{
    return r.left() >= left() &&
           r.left().as_int() + r.size.width.as_int() <=
               left().as_int() + size.width.as_int() &&
           r.top() >= top() &&
           r.top().as_int() + r.size.height.as_int() <=
               top().as_int() + size.height.as_int();
}

bool geom::Rectangle::contains(Point const& p) const
{
    if (size.width == geom::Width{0} || size.height == geom::Height{0})
        return false;

    auto br = bottom_right();
    return p.x >= left() && p.x < br.x &&
           p.y >= top() && p.y < br.y;
}

bool geom::Rectangle::overlaps(Rectangle const& r) const
{
    bool disjoint = r.left() >= right()
                 || r.right() <= left()
                 || r.top() >= bottom()
                 || r.bottom() <= top()
                 || size.width == geom::Width{0}
                 || size.height == geom::Height{0}
                 || r.size.width == geom::Width{0}
                 || r.size.height == geom::Height{0};
    return !disjoint;
}

geom::Rectangle geom::Rectangle::intersection_with(Rectangle const& r) const
{
    auto const max_left   = std::max(left(),   r.left());
    auto const min_right  = std::min(right(),  r.right());
    auto const max_top    = std::max(top(),    r.top());
    auto const min_bottom = std::min(bottom(), r.bottom());

    if (max_left < min_right && max_top < min_bottom)
        return {{max_left, max_top},
                {(min_right - max_left).as_int(),
                 (min_bottom - max_top).as_int()}};
    else
        return geom::Rectangle();
}
