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

#ifndef MIR_GEOMETRY_SIZE_H_
#define MIR_GEOMETRY_SIZE_H_

#include "size_generic.h"
#include "dimensions.h"

namespace mir
{
namespace geometry
{
struct Point;
struct Displacement;
struct Size : generic::Size<detail::IntWrapper>
{
    using PointType = Point;
    using DisplacementType = Displacement;
    using generic::Size<detail::IntWrapper>::Size;
};
}
}

#endif /* MIR_GEOMETRY_SIZE_H_ */
