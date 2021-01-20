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
 * Authored by: William Wold <william.wold@canonical.com
 */

#ifndef MIR_GEOMETRY_POINT_F_H_
#define MIR_GEOMETRY_POINT_F_H_

#include "dimensions_f.h"
#include "point_generic.h"
#include <iosfwd>

namespace mir
{
namespace geometry
{
using PointF = generic::Point<generic::Value<float>::Wrapper>;
}
}
#endif // MIR_GEOMETRY_POINT_F_H_
