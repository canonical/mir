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
    return r.top_left.x >= top_left.x &&
           r.top_left.x.as_int() + r.size.width.as_int() <=
               top_left.x.as_int() + size.width.as_int() &&
           r.top_left.y >= top_left.y &&
           r.top_left.y.as_int() + r.size.height.as_int() <=
               top_left.y.as_int() + size.height.as_int();
}

bool geom::Rectangle::contains(Point const& p) const
{
    if (size.width == geom::Width{0} || size.height == geom::Height{0})
        return false;

    auto br = bottom_right();
    return p.x >= top_left.x && p.x < br.x &&
           p.y >= top_left.y && p.y < br.y;
}

bool geom::Rectangle::overlaps(Rectangle const& r) const
{
    if (size.width > geom::Width{0} && size.height > geom::Height{0} &&
        r.size.width > geom::Width{0} && r.size.height > geom::Height{0})
    {
        auto tl1 = top_left;
        auto br1 = bottom_right();
        auto tl2 = r.top_left;
        auto br2 = r.bottom_right();

        return !(tl2.x >= br1.x || br2.x <= tl1.x ||
                 tl2.y >= br1.y || br2.y <= tl1.y);
    }
    else
    {
        return false;
    }
}

geom::Rectangle geom::Rectangle::intersection_with(Rectangle const& r) const
{
    int const a = std::max(top_left.x.as_int(), r.top_left.x.as_int());
    int const b = std::min(bottom_right().x.as_int(), r.bottom_right().x.as_int());
    int const c = std::max(top_left.y.as_int(), r.top_left.y.as_int());
    int const d = std::min(bottom_right().y.as_int(), r.bottom_right().y.as_int());

    if (a < b && c < d)
        return {{a, c}, {b - a, d - c}};
    else
        return geom::Rectangle();
}
