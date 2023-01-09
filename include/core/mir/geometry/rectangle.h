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

#ifndef MIR_GEOMETRY_RECTANGLE_H_
#define MIR_GEOMETRY_RECTANGLE_H_

#include "forward.h"
#include "point.h"
#include "size.h"
#include "displacement.h"

#include <ostream>

namespace mir
{
namespace geometry
{
namespace generic
{
template<typename T>
struct Rectangle
{
    constexpr Rectangle() = default;

    constexpr Rectangle(Point<T> const& top_left, Size<T> const& size)
        : top_left{top_left}, size{size}
    {
    }

    /**
     * The bottom right boundary point of the rectangle.
     *
     * Note that the returned point is *not* included in the rectangle
     * area, that is, the rectangle is represented as [top_left,bottom_right).
     */
    Point<T> bottom_right() const
    {
        return top_left + as_displacement(size);
    }

    Point<T> top_right() const
    {
        return top_left + as_delta(size.width);
    }

    Point<T> bottom_left() const
    {
        return top_left + as_delta(size.height);
    }

    bool contains(Point<T> const& p) const
    {
        if (size.width == decltype(size.width){} || size.height == decltype(size.height){})
            return false;

        auto br = bottom_right();
        return p.x >= left() && p.x < br.x &&
               p.y >= top() && p.y < br.y;
    }

    /**
     * Test if the rectangle contains another.
     *
     * Note that an empty rectangle can still contain other empty rectangles,
     * which are treated as points or lines of thickness zero.
     */
    bool contains(Rectangle<T> const& r) const
    {
        return r.left() >= left() &&
               r.left() + as_delta(r.size.width) <= left() + as_delta(size.width) &&
               r.top() >= top() &&
               r.top() + as_delta(r.size.height) <= top() + as_delta(size.height);
    }

    bool overlaps(Rectangle<T> const& r) const
    {
        bool disjoint = r.left() >= right()
                     || r.right() <= left()
                     || r.top() >= bottom()
                     || r.bottom() <= top()
                     || size.width == decltype(size.width){}
                     || size.height == decltype(size.height){}
                     || r.size.width == decltype(r.size.width){}
                     || r.size.height == decltype(r.size.height){};
        return !disjoint;
    }

    X<T> left() const   { return top_left.x; }
    X<T> right() const  { return bottom_right().x; }
    Y<T> top() const    { return top_left.y; }
    Y<T> bottom() const { return bottom_right().y; }

    Point<T> top_left;
    Size<T> size;
};

template<typename T>
Rectangle<T> intersection_of(Rectangle<T> const& a, Rectangle<T> const& b)
{
    auto const max_left   = std::max(a.left(),   b.left());
    auto const min_right  = std::min(a.right(),  b.right());
    auto const max_top    = std::max(a.top(),    b.top());
    auto const min_bottom = std::min(a.bottom(), b.bottom());

    if (max_left < min_right && max_top < min_bottom)
        return {{max_left, max_top},
                {(min_right - max_left).as_value(),
                (min_bottom - max_top).as_value()}};
    else
        return {};
}

template<typename T>
inline constexpr bool operator == (Rectangle<T> const& lhs, Rectangle<T> const& rhs)
{
    return lhs.top_left == rhs.top_left && lhs.size == rhs.size;
}

template<typename T>
inline constexpr bool operator != (Rectangle<T> const& lhs, Rectangle<T> const& rhs)
{
    return lhs.top_left != rhs.top_left || lhs.size != rhs.size;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, Rectangle<T> const& value)
{
    out << '(' << value.top_left << ", " << value.size << ')';
    return out;
}
}
}
}

#endif // MIR_GEOMETRY_RECTANGLE_H_
