/*
 * Copyright © 2021 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_GEOMETRY_SIZE_GENERIC_H_
#define MIR_GEOMETRY_SIZE_GENERIC_H_

#include <iostream>

namespace mir
{
namespace geometry
{
namespace detail
{
struct PointBase;
struct SizeBase{}; ///< Used for determining if a type is a size
}
namespace generic
{
template<template<typename> typename T>
struct Point;

template<template<typename> typename T>
struct Size : detail::SizeBase
{
    template<typename Tag>
    using WrapperType = T<Tag>;

    using PointType = Point<T>;

    constexpr Size() noexcept {}
    constexpr Size(Size const&) noexcept = default;
    Size& operator=(Size const&) noexcept = default;

    template<typename WidthType, typename HeightType>
    constexpr Size(WidthType&& width, HeightType&& height) noexcept : width(width), height(height) {}

    T<WidthTag> width;
    T<HeightTag> height;
};

template<typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
inline constexpr bool operator == (S const& lhs, S const& rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height;
}

template<typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
inline constexpr bool operator != (S const& lhs, S const& rhs)
{
    return lhs.width != rhs.width || lhs.height != rhs.height;
}

template<typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
std::ostream& operator<<(std::ostream& out, S const& value)
{
    out << '(' << value.width << ", " << value.height << ')';
    return out;
}

template<typename Scalar, typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
inline constexpr S operator*(Scalar scale, S const& size)
{
    return S{scale*size.width, scale*size.height};
}

template<typename Scalar, typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
inline constexpr S operator*(S const& size, Scalar scale)
{
    return scale*size;
}

template<typename Scalar, typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
inline constexpr S operator/(S const& size, Scalar scale)
{
    return S{size.width / scale, size.height / scale};
}

template<typename P, typename std::enable_if<std::is_base_of<detail::PointBase, P>::value, bool>::type = true>
inline constexpr typename P::SizeType as_size(P const& point)
{
    return typename P::SizeType{point.x.as_int(), point.y.as_int()};
}

template<typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
inline constexpr typename S::PointType as_point(S const& size)
{
    return typename S::PointType{size.width.as_int(), size.height.as_int()};
}
}
}
}

#endif // MIR_GEOMETRY_SIZE_GENERIC_H_
