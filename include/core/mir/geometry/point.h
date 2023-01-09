/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GEOMETRY_POINT_H_
#define MIR_GEOMETRY_POINT_H_

#include "forward.h"
#include "dimensions.h"
#include <ostream>

namespace mir
{
namespace geometry
{
namespace generic
{
template<typename T>
struct Size;
template<typename T>
struct Displacement;

template<typename T>
struct Point
{
    using ValueType = T;

    constexpr Point() = default;
    constexpr Point(Point const&) = default;
    Point& operator=(Point const&) = default;

    template<typename U>
    explicit constexpr Point(Point<U> const& other) noexcept
        : x{X<T>{other.x}},
          y{Y<T>{other.y}}
    {
    }

    template<typename XType, typename YType>
    constexpr Point(XType&& x, YType&& y) : x(x), y(y) {}

    X<T> x;
    Y<T> y;
};

template<typename T>
inline constexpr bool operator == (Point<T> const& lhs, Point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template<typename T>
inline constexpr bool operator != (Point<T> const& lhs, Point<T> const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

template<typename T>
inline constexpr Point<T> operator+(Point<T> lhs, DeltaX<T> rhs) { return{lhs.x + rhs, lhs.y}; }
template<typename T>
inline constexpr Point<T> operator+(Point<T> lhs, DeltaY<T> rhs) { return{lhs.x, lhs.y + rhs}; }

template<typename T>
inline constexpr Point<T> operator-(Point<T> lhs, DeltaX<T> rhs) { return{lhs.x - rhs, lhs.y}; }
template<typename T>
inline constexpr Point<T> operator-(Point<T> lhs, DeltaY<T> rhs) { return{lhs.x, lhs.y - rhs}; }

template<typename T>
inline Point<T>& operator+=(Point<T>& lhs, DeltaX<T> rhs) { lhs.x += rhs; return lhs; }
template<typename T>
inline Point<T>& operator+=(Point<T>& lhs, DeltaY<T> rhs) { lhs.y += rhs; return lhs; }

template<typename T>
inline Point<T>& operator-=(Point<T>& lhs, DeltaX<T> rhs) { lhs.x -= rhs; return lhs; }
template<typename T>
inline Point<T>& operator-=(Point<T>& lhs, DeltaY<T> rhs) { lhs.y -= rhs; return lhs; }

template<typename T>
std::ostream& operator<<(std::ostream& out, Point<T> const& value)
{
    out << value.x << ", " << value.y;
    return out;
}

}
}
}

#endif // MIR_GEOMETRY_POINT_H_
