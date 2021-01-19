/*
 * Copyright © 2020 Canonical Ltd.
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

#ifndef MIR_GEOMETRY_POINT_GENERIC_H_
#define MIR_GEOMETRY_POINT_GENERIC_H_

#include "dimensions_generic.h"

namespace mir
{
namespace geometry
{
namespace generic
{
struct PointBase
{
};

template<template<typename> typename T>
struct Point : PointBase
{
    template<typename Tag>
    using WrapperType = T<Tag>;

    constexpr Point() = default;
    constexpr Point(Point const&) = default;
    Point& operator=(Point const&) = default;

    template<typename XType, typename YType>
    constexpr Point(XType&& x, YType&& y) : x(x), y(y) {}

    T<XTag> x;
    T<YTag> y;
};

template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline constexpr bool operator == (P const& lhs, P const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline constexpr bool operator != (P const& lhs, P const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline constexpr P operator+(P lhs, typename P::template WrapperType<DeltaXTag> rhs) { return{lhs.x + rhs, lhs.y}; }
template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline constexpr P operator+(P lhs, typename P::template WrapperType<DeltaYTag> rhs) { return{lhs.x, lhs.y + rhs}; }

template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline constexpr P operator-(P lhs, typename P::template WrapperType<DeltaXTag> rhs) { return{lhs.x - rhs, lhs.y}; }
template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline constexpr P operator-(P lhs, typename P::template WrapperType<DeltaYTag> rhs) { return{lhs.x, lhs.y - rhs}; }

template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline P& operator+=(P& lhs, typename P::template WrapperType<DeltaXTag> rhs) { lhs.x += rhs; return lhs; }
template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline P& operator+=(P& lhs, typename P::template WrapperType<DeltaYTag> rhs) { lhs.y += rhs; return lhs; }

template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline P& operator-=(P& lhs, typename P::template WrapperType<DeltaXTag> rhs) { lhs.x -= rhs; return lhs; }
template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
inline P& operator-=(P& lhs, typename P::template WrapperType<DeltaYTag> rhs) { lhs.y -= rhs; return lhs; }

template<typename P, typename std::enable_if<std::is_base_of<PointBase, P>::value, bool>::type = true>
std::ostream& operator<<(std::ostream& out, P const& value)
{
    out << value.x << ", " << value.y;
    return out;
}

}
}
}

#endif // MIR_GEOMETRY_POINT_GENERIC_H_
