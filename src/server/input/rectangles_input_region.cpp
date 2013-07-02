/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/input/rectangles_input_region.h"
#include "mir/geometry/rectangle.h"

namespace mi = mir::input;
namespace geom = mir::geometry;

mi::RectanglesInputRegion::RectanglesInputRegion(std::initializer_list<geom::Rectangle> const& input_rectangles)
    : input_rectangles(input_rectangles.begin(), input_rectangles.end())
{
}

namespace
{

bool rectangle_contains_point(geom::Rectangle const& rectangle, uint32_t px, uint32_t py)
{
    auto width = rectangle.size.width.as_uint32_t();
    auto height = rectangle.size.height.as_uint32_t();
    auto x = rectangle.top_left.x.as_uint32_t();
    auto y = rectangle.top_left.y.as_uint32_t();
    
    if (px < x)
        return false;
    else if (py <  y)
        return false;
    else if (px > x + width)
        return false;
    else if (py > y + height)
        return false;
    return true;
}

}

bool mi::RectanglesInputRegion::contains(geom::Point const& point) const
{
    for (auto const& rectangle : input_rectangles)
    {
        if (rectangle_contains_point(rectangle, point.x.as_uint32_t(), point.y.as_uint32_t()))
            return true;
    }
    return false;
}
