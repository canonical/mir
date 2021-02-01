/*
 * Copyright © 2012, 2016 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GEOMETRY_POINT_H_
#define MIR_GEOMETRY_POINT_H_

#include "dimensions.h"
#include "point_generic.h"
#include <iosfwd>

namespace mir
{
namespace geometry
{
struct Point : generic::Point<detail::IntWrapper>
{
    template<typename Tag>
    using WrapperType = detail::IntWrapper<Tag>;

    using generic::Point<detail::IntWrapper>::Point;

    constexpr Point() = default;
    constexpr Point(Point const&) = default;
    Point& operator=(Point const&) = default;
};
}
}
#endif /* MIR_GEOMETRY_POINT_H_ */
