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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GEOMETRY_POINT_H_
#define MIR_GEOMETRY_POINT_H_

#include "dimensions.h"
#include <iosfwd>

namespace mir
{
namespace geometry
{

struct Point
{
    Point() = default;
    Point(Point const&) = default;
    Point& operator=(Point const&) = default;

    template<typename XType, typename YType>
    Point(XType&& x, YType&& y) : x(x), y(y) {}

    X x;
    Y y;
};

struct PointOffset
{
    PointOffset() = default;
    PointOffset(PointOffset const&) = default;
    PointOffset& operator=(PointOffset const&) = default;

    template<typename DeltaXType, typename DeltaYType>
    PointOffset(DeltaXType&& dx, DeltaYType&& dy) : dx{dx}, dy{dy} {}

    DeltaX dx;
    DeltaY dy;
};

inline bool operator == (Point const& lhs, Point const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator != (Point const& lhs, Point const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

inline bool operator == (PointOffset const& lhs, PointOffset const& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}

inline bool operator != (PointOffset const& lhs, PointOffset const& rhs)
{
    return lhs.dx != rhs.dx || lhs.dy != rhs.dy;
}

inline Point operator+(Point lhs, PointOffset rhs) { return {lhs.x - rhs.dx, lhs.y - rhs.dy}; }
inline Point operator+(Point lhs, DeltaX rhs) { return{lhs.x + rhs, lhs.y}; }
inline Point operator+(Point lhs, DeltaY rhs) { return{lhs.x, lhs.y + rhs}; }

inline Point operator-(Point lhs, PointOffset rhs) { return {lhs.x - rhs.dx, lhs.y - rhs.dy}; }
inline Point operator-(Point lhs, DeltaX rhs) { return{lhs.x - rhs, lhs.y}; }
inline Point operator-(Point lhs, DeltaY rhs) { return{lhs.x, lhs.y - rhs}; }

inline Point& operator+=(Point& lhs, DeltaX rhs) { lhs.x += rhs; return lhs; }
inline Point& operator+=(Point& lhs, DeltaY rhs) { lhs.y += rhs; return lhs; }

inline Point& operator-=(Point& lhs, DeltaX rhs) { lhs.x -= rhs; return lhs; }
inline Point& operator-=(Point& lhs, DeltaY rhs) { lhs.y -= rhs; return lhs; }

std::ostream& operator<<(std::ostream& out, Point const& value);
}
}

#endif /* MIR_GEOMETRY_POINT_H_ */
