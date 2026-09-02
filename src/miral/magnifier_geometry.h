/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_MAGNIFIER_GEOMETRY_H
#define MIRAL_MAGNIFIER_GEOMETRY_H

#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/size.h>

namespace miral::magnifier_geometry
{
inline auto visual_bounds(
    mir::geometry::Point const& surface_top_left,
    mir::geometry::Size const& capture_size,
    double const magnification) -> mir::geometry::RectangleD
{
    auto const center = mir::geometry::PointD{
        surface_top_left.x.as_value() + capture_size.width.as_value() / 2.0,
        surface_top_left.y.as_value() + capture_size.height.as_value() / 2.0,
    };

    auto const visual_size = mir::geometry::SizeD{
        capture_size.width.as_value() * magnification,
        capture_size.height.as_value() * magnification,
    };

    return {
        {
            center.x.as_value() - visual_size.width.as_value() / 2.0,
            center.y.as_value() - visual_size.height.as_value() / 2.0,
        },
        visual_size,
    };
}
}

#endif
