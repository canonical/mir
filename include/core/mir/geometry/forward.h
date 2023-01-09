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

#ifndef MIR_GEOMETRY_FORWARD_H_
#define MIR_GEOMETRY_FORWARD_H_

namespace mir
{
/// Basic geometry types. Types for dimensions, displacements, etc.
/// and the operations that they support.
namespace geometry
{
/// These tag types determine what type of dimension a value holds and what operations are possible with it. They are
/// only used as template parameters, are never instantiated and should only require forward declarations, but some
/// compiler versions seem to fail if they aren't given real declarations.
/// @{
struct WidthTag{};
struct HeightTag{};
struct XTag{};
struct YTag{};
struct DeltaXTag{};
struct DeltaYTag{};
struct StrideTag{};
/// @}

namespace generic
{
template<typename T, typename Tag>
struct Value;

template<typename T>
struct Point;

template<typename T>
struct Size;

template<typename T>
struct Displacement;

template<typename T>
struct Rectangle;

template<typename T> using Width = Value<T, WidthTag>;
template<typename T> using Height = Value<T, HeightTag>;
template<typename T> using X = Value<T, XTag>;
template<typename T> using Y = Value<T, YTag>;
template<typename T> using DeltaX = Value<T, DeltaXTag>;
template<typename T> using DeltaY = Value<T, DeltaYTag>;
}

using Width = generic::Width<int>;
using Height = generic::Height<int>;
using X = generic::X<int>;
using Y = generic::Y<int>;
using DeltaX = generic::DeltaX<int>;
using DeltaY = generic::DeltaY<int>;

using WidthF = generic::Width<float>;
using HeightF = generic::Height<float>;
using XF = generic::X<float>;
using YF = generic::Y<float>;
using DeltaXF = generic::DeltaX<float>;
using DeltaYF = generic::DeltaY<float>;

// Just to be clear, mir::geometry::Stride is the stride of the buffer in bytes
using Stride = generic::Value<int, StrideTag>;

using Point = generic::Point<int>;
using Size = generic::Size<int>;
using Displacement = generic::Displacement<int>;
using Rectangle = generic::Rectangle<int>;

using PointF = generic::Point<float>;
using SizeF = generic::Size<float>;
using DisplacementF = generic::Displacement<float>;
using RectangleF = generic::Rectangle<int>;
}
}

#endif // MIR_GEOMETRY_FORWARD_H_
