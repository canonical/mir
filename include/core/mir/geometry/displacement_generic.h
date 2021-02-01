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

#ifndef MIR_GEOMETRY_DISPLACEMENT_GENERIC_H_
#define MIR_GEOMETRY_DISPLACEMENT_GENERIC_H_

#include "dimensions_generic.h"
#include <iostream>

namespace mir
{
namespace geometry
{
namespace detail
{
struct PointBase;
struct SizeBase;
struct DisplacementBase{}; ///< Used for determining if a type is a displacement
}
namespace generic
{
template<template<typename> typename T>
struct Point;

template<template<typename> typename T>
struct Size;

template<template<typename> typename T>
struct Displacement : detail::DisplacementBase
{
    template<typename Tag>
    using WrapperType = T<Tag>;

    using PointType = Point<T>;
    using SizeType = Size<T>;

    constexpr Displacement() {}
    constexpr Displacement(Displacement const&) = default;
    Displacement& operator=(Displacement const&) = default;

    template<typename DeltaXType, typename DeltaYType>
    constexpr Displacement(DeltaXType&& dx, DeltaYType&& dy) : dx{dx}, dy{dy} {}

    T<DeltaXTag> dx;
    T<DeltaYTag> dy;
};

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr bool operator==(D const& lhs, D const& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr bool operator!=(D const& lhs, D const& rhs)
{
    return lhs.dx != rhs.dx || lhs.dy != rhs.dy;
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
std::ostream& operator<<(std::ostream& out, D const& value)
{
    out << '(' << value.dx << ", " << value.dy << ')';
    return out;
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr D operator+(D const& lhs, D const& rhs)
{
    return D{lhs.dx + rhs.dx, lhs.dy + rhs.dy};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr D operator-(D const& lhs, D const& rhs)
{
    return D{lhs.dx - rhs.dx, lhs.dy - rhs.dy};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr D operator-(D const& rhs)
{
    return D{-rhs.dx, -rhs.dy};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr typename D::PointType operator+(typename D::PointType const& lhs, D const& rhs)
{
    return typename D::PointType{lhs.x + rhs.dx, lhs.y + rhs.dy};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr typename D::PointType operator+(D const& lhs, typename D::PointType const& rhs)
{
    return typename D::PointType{rhs.x + lhs.dx, rhs.y + lhs.dy};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr typename D::PointType operator-(typename D::PointType const& lhs, D const& rhs)
{
    return typename D::PointType{lhs.x - rhs.dx, lhs.y - rhs.dy};
}

template<typename P, typename std::enable_if<std::is_base_of<detail::PointBase, P>::value, bool>::type = true>
inline constexpr typename P::DisplacementType operator-(P const& lhs, P const& rhs)
{
    return typename P::DisplacementType{lhs.x - rhs.x, lhs.y - rhs.y};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr typename D::PointType& operator+=(typename D::PointType& lhs, D const& rhs)
{
    return lhs = lhs + rhs;
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr typename D::PointType& operator-=(typename D::PointType& lhs, D const& rhs)
{
    return lhs = lhs - rhs;
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline bool operator<(D const& lhs, D const& rhs)
{
    return lhs.length_squared() < rhs.length_squared();
}

template<typename Scalar, typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr D operator*(Scalar scale, D const& disp)
{
    return D{scale*disp.dx, scale*disp.dy};
}

template<typename Scalar, typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr D operator*(D const& disp, Scalar scale)
{
    return scale*disp;
}

template<typename S, typename std::enable_if<std::is_base_of<detail::SizeBase, S>::value, bool>::type = true>
inline constexpr typename S::DisplacementType as_displacement(S const& size)
{
    return typename S::DisplacementType{size.width.as_int(), size.height.as_int()};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr typename D::SizeType as_size(D const& disp)
{
    return typename D::SizeType{disp.dx.as_int(), disp.dy.as_int()};
}

template<typename P, typename std::enable_if<std::is_base_of<detail::PointBase, P>::value, bool>::type = true>
inline constexpr typename P::DisplacementType as_displacement(P const& point)
{
    return typename P::DisplacementType{point.x.as_int(), point.y.as_int()};
}

template<typename D, typename std::enable_if<std::is_base_of<detail::DisplacementBase, D>::value, bool>::type = true>
inline constexpr typename D::PointType as_point(D const& disp)
{
    return typename D::PointType{disp.dx.as_int(), disp.dy.as_int()};
}
}
}
}

#endif // MIR_GEOMETRY_DISPLACEMENT_GENERIC_H_
