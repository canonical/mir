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

#ifndef MIR_GEOMETRY_DISPLACEMENT_H_
#define MIR_GEOMETRY_DISPLACEMENT_H_

#include "forward.h"
#include "dimensions.h"
#include "point.h"
#include <ostream>

namespace mir
{
namespace geometry
{
namespace generic
{
template<typename T>
struct Point;

template<typename T>
struct Size;

template<typename T>
struct Displacement
{
    using ValueType = T;

    constexpr Displacement() {}
    constexpr Displacement(Displacement const&) = default;
    Displacement& operator=(Displacement const&) = default;

    template<typename U>
    explicit constexpr Displacement(Displacement<U> const& other) noexcept
        : dx{DeltaX<T>{other.dx}},
          dy{DeltaY<T>{other.dy}}
    {
    }

    template<typename DeltaXType, typename DeltaYType>
    constexpr Displacement(DeltaXType&& dx, DeltaYType&& dy) : dx{dx}, dy{dy} {}

    template <typename Q = T>
    constexpr typename std::enable_if<std::is_integral<Q>::value, long long>::type length_squared() const
    {
        long long x = dx.as_value(), y = dy.as_value();
        return x * x + y * y;
    }

    template <typename Q = T>
    constexpr typename std::enable_if<!std::is_integral<Q>::value, T>::type length_squared() const
    {
        T x = dx.as_value(), y = dy.as_value();
        return x * x + y * y;
    }

    DeltaX<T> dx;
    DeltaY<T> dy;
};

template<typename T>
inline constexpr bool operator==(Displacement<T> const& lhs, Displacement<T> const& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}

template<typename T>
inline constexpr bool operator!=(Displacement<T> const& lhs, Displacement<T> const& rhs)
{
    return lhs.dx != rhs.dx || lhs.dy != rhs.dy;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, Displacement<T> const& value)
{
    out << '(' << value.dx << ", " << value.dy << ')';
    return out;
}

template<typename T>
inline constexpr Displacement<T> operator+(Displacement<T> const& lhs, Displacement<T> const& rhs)
{
    return Displacement<T>{lhs.dx + rhs.dx, lhs.dy + rhs.dy};
}

template<typename T>
inline constexpr Displacement<T> operator-(Displacement<T> const& lhs, Displacement<T> const& rhs)
{
    return Displacement<T>{lhs.dx - rhs.dx, lhs.dy - rhs.dy};
}

template<typename T>
inline constexpr Displacement<T> operator-(Displacement<T> const& rhs)
{
    return Displacement<T>{-rhs.dx, -rhs.dy};
}

template<typename T>
inline constexpr Point<T> operator+(Point<T> const& lhs, Displacement<T> const& rhs)
{
    return Point<T>{lhs.x + rhs.dx, lhs.y + rhs.dy};
}

template<typename T>
inline constexpr Point<T> operator+(Displacement<T> const& lhs, Point<T> const& rhs)
{
    return Point<T>{rhs.x + lhs.dx, rhs.y + lhs.dy};
}

template<typename T>
inline constexpr Point<T> operator-(Point<T> const& lhs, Displacement<T> const& rhs)
{
    return Point<T>{lhs.x - rhs.dx, lhs.y - rhs.dy};
}

template<typename T>
inline constexpr Displacement<T> operator-(Point<T> const& lhs, Point<T> const& rhs)
{
    return Displacement<T>{lhs.x - rhs.x, lhs.y - rhs.y};
}

template<typename T>
inline constexpr Point<T>& operator+=(Point<T>& lhs, Displacement<T> const& rhs)
{
    return lhs = lhs + rhs;
}

template<typename T>
inline constexpr Point<T>& operator-=(Point<T>& lhs, Displacement<T> const& rhs)
{
    return lhs = lhs - rhs;
}

template<typename T>
inline bool operator<(Displacement<T> const& lhs, Displacement<T> const& rhs)
{
    return lhs.length_squared() < rhs.length_squared();
}

template<typename T, typename Scalar>
inline constexpr Displacement<T> operator*(Scalar scale, Displacement<T> const& disp)
{
    return Displacement<T>{scale*disp.dx, scale*disp.dy};
}

template<typename T, typename Scalar>
inline constexpr Displacement<T> operator*(Displacement<T> const& disp, Scalar scale)
{
    return scale*disp;
}

template<typename T>
inline constexpr Displacement<T> as_displacement(Size<T> const& size)
{
    return Displacement<T>{size.width.as_value(), size.height.as_value()};
}

template<typename T>
inline constexpr Size<T> as_size(Displacement<T> const& disp)
{
    return Size<T>{disp.dx.as_value(), disp.dy.as_value()};
}

template<typename T>
inline constexpr Displacement<T> as_displacement(Point<T> const& point)
{
    return Displacement<T>{point.x.as_value(), point.y.as_value()};
}

template<typename T>
inline constexpr Point<T> as_point(Displacement<T> const& disp)
{
    return Point<T>{disp.dx.as_value(), disp.dy.as_value()};
}
}
}
}

#endif // MIR_GEOMETRY_DISPLACEMENT_H_
