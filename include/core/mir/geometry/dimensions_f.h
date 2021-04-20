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

#ifndef MIR_GEOMETRY_DIMENSIONS_F_H_
#define MIR_GEOMETRY_DIMENSIONS_F_H_

#include "dimensions_generic.h"

namespace mir
{
namespace geometry
{
using WidthF = generic::Width<float>;
using HeightF = generic::Height<float>;
using XF = generic::X<float>;
using YF = generic::Y<float>;
using DeltaXF = generic::DeltaX<float>;
using DeltaYF = generic::DeltaY<float>;
}
}

#endif // MIR_GEOMETRY_DIMENSIONS_F_H_
