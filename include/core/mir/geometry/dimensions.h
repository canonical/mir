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

#ifndef MIR_GEOMETRY_DIMENSIONS_H_
#define MIR_GEOMETRY_DIMENSIONS_H_

#include "forward.h"
#include "dimensions_generic.h"

#include <cstdint>

namespace mir
{
/// Basic geometry types. Types for dimensions, displacements, etc.
/// and the operations that they support.
namespace geometry
{
using Width = generic::Width<int>;
using Height = generic::Height<int>;
// Just to be clear, mir::geometry::Stride is the stride of the buffer in bytes
using Stride = generic::Value<int>::Wrapper<struct StrideTag>;

using X = generic::X<int>;
using Y = generic::Y<int>;
using DeltaX = generic::DeltaX<int>;
using DeltaY = generic::DeltaY<int>;
}
}

#endif /* MIR_GEOMETRY_DIMENSIONS_H_ */
