/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GEOMETRY_RECTANGLE_H_
#define MIR_GEOMETRY_RECTANGLE_H_

#include "mir/geometry/point.h"
#include "size.h"

#include <iosfwd>

namespace mir
{
namespace geometry
{

struct Rectangle
{
    Rectangle() = default;

    Rectangle(Point const& top_left, Size const& size)
        : top_left{top_left}, size{size}
    {
    }

    Point top_left;
    Size size;

    /**
     * The bottom right boundary point of the rectangle.
     *
     * Note that the returned point is *not* included in the rectangle
     * area, that is, the rectangle is represented as [top_left,bottom_right).
     */
    Point bottom_right() const;
    Point top_right() const;
    Point bottom_left() const;
    bool contains(Point const& p) const;

    /**
     * Test if the rectangle contains another.
     *
     * Note that an empty rectangle can still contain other empty rectangles,
     * which are treated as points or lines of thickness zero.
     */
    bool contains(Rectangle const& r) const;

    bool overlaps(Rectangle const& r) const;

    Rectangle intersection_with(Rectangle const& r) const;
};

inline bool operator == (Rectangle const& lhs, Rectangle const& rhs)
{
    return lhs.top_left == rhs.top_left && lhs.size == rhs.size;
}

inline bool operator != (Rectangle const& lhs, Rectangle const& rhs)
{
    return lhs.top_left != rhs.top_left || lhs.size != rhs.size;
}

std::ostream& operator<<(std::ostream& out, Rectangle const& value);
}
}

#endif /* MIR_GEOMETRY_RECTANGLE_H_ */
