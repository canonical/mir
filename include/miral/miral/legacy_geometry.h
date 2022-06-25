/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_LEGACY_GEOMETRY_H_
#define MIRAL_LEGACY_GEOMETRY_H_

// Prevents new Point, Rectangle, etc from being defined as typedefs to the generic version
#define LEGACY_MIRAL_GEOMETRY_TYPES

#include <mir/geometry/point.h>
#include <mir/geometry/size.h>
#include <mir/geometry/displacement.h>
#include <mir/geometry/rectangle.h>

namespace mir
{
namespace geometry
{

class Point: public generic::Point<int>
{
public:
    template<typename... Args>
    constexpr Point(Args... args) noexcept : generic::Point<int>{args...} {}
};

class Size: public generic::Size<int>
{
public:
    template<typename... Args>
    constexpr Size(Args... args) noexcept : generic::Size<int>{args...} {}
};

class Displacement: public generic::Displacement<int>
{
public:
    template<typename... Args>
    constexpr Displacement(Args... args) noexcept : generic::Displacement<int>{args...} {}
};

class Rectangle: public generic::Rectangle<int>
{
public:
    template<typename... Args>
    constexpr Rectangle(Args... args) noexcept : generic::Rectangle<int>{args...} {}

    constexpr Rectangle(Point a, Size b) noexcept : generic::Rectangle<int>{a, b} {}
};

}
}

#endif // MIRAL_LEGACY_GEOMETRY_H_
