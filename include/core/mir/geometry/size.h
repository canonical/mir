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

#ifndef MIR_GEOMETRY_SIZE_H_
#define MIR_GEOMETRY_SIZE_H_

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
struct Point;
template<typename T>
struct Displacement;

template<typename T>
struct Size
{
    using ValueType = T;

    constexpr Size() noexcept {}
    constexpr Size(Size const&) noexcept = default;
    Size& operator=(Size const&) noexcept = default;

    template<typename U>
    explicit constexpr Size(Size<U> const& other) noexcept
        : width{Width<T>{other.width}},
          height{Height<T>{other.height}}
    {
    }

    template<typename WidthType, typename HeightType>
    constexpr Size(WidthType&& width, HeightType&& height) noexcept : width(width), height(height) {}

    Width<T> width;
    Height<T> height;
};

template<typename T>
inline constexpr bool operator == (Size<T> const& lhs, Size<T> const& rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height;
}

template<typename T>
inline constexpr bool operator != (Size<T> const& lhs, Size<T> const& rhs)
{
    return lhs.width != rhs.width || lhs.height != rhs.height;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, Size<T> const& value)
{
    out << '(' << value.width << ", " << value.height << ')';
    return out;
}

template<typename T, typename Scalar>
inline constexpr Size<T> operator*(Scalar scale, Size<T> const& size)
{
    return Size<T>{scale*size.width, scale*size.height};
}

template<typename T, typename Scalar>
inline constexpr Size<T> operator*(Size<T> const& size, Scalar scale)
{
    return scale*size;
}

template<typename T, typename Scalar>
inline constexpr Size<T> operator/(Size<T> const& size, Scalar scale)
{
    return Size<T>{size.width / scale, size.height / scale};
}

template<typename T>
inline constexpr Size<T> as_size(Point<T> const& point)
{
    return Size<T>{point.x.as_value(), point.y.as_value()};
}

template<typename T>
inline constexpr Point<T> as_point(Size<T> const& size)
{
    return Point<T>{size.width.as_value(), size.height.as_value()};
}
}
}
}

#endif // MIR_GEOMETRY_SIZE_H_
