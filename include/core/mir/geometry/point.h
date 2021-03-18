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

#include "point_generic.h"
#include "dimensions.h"

namespace mir
{
namespace geometry
{
struct Size;
struct Displacement;
struct Point : generic::Point<detail::IntWrapper>
{
    using SizeType = Size;
    using DisplacementType = Displacement;
    using generic::Point<detail::IntWrapper>::Point;
};
}
}
#endif /* MIR_GEOMETRY_POINT_H_ */
