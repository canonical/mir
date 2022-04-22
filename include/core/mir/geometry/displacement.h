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
 */

#ifndef MIR_GEOMETRY_DISPLACEMENT_H_
#define MIR_GEOMETRY_DISPLACEMENT_H_

#include "displacement_generic.h"
#include "dimensions.h"
#include "point.h"
#include "size.h"

namespace mir
{
namespace geometry
{
struct Displacement : generic::Displacement<detail::IntWrapper>
{
    using PointType = Point;
    using SizeType = Size;

    using generic::Displacement<detail::IntWrapper>::Displacement;

    long long length_squared() const
    {
        long long x = dx.as_int(), y = dy.as_int();
        return x * x + y * y;
    }
};
}
}

#endif /* MIR_GEOMETRY_DISPLACEMENT_H_ */
