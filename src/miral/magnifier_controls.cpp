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

#include "magnifier_controls.h"

#include "magnifier_geometry.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

namespace geom = mir::geometry;
namespace controls = miral::magnifier_controls;

auto controls::Positions::for_kind(HandleKind const kind) const -> geom::Point
{
    switch (kind)
    {
    case HandleKind::drag:
        return drag;
    case HandleKind::resize:
        return resize;
    case HandleKind::zoom_in:
        return zoom_in;
    case HandleKind::zoom_out:
        return zoom_out;
    }

    std::unreachable();
}

auto controls::positions_for(miral::magnifier_layout::FreePlacement const& placement, double const magnification)
    -> Positions
{
    auto const bounds = miral::magnifier_geometry::visual_bounds(
        placement.surface_top_left, placement.capture_area.size, magnification);

    auto const left = geom::X{bounds.left()};
    auto const right = geom::X{bounds.right()};
    auto const top = geom::Y{bounds.top()};
    auto const bottom = geom::Y{bounds.bottom()};

    auto const handle_width = geom::DeltaX{handle_diameter};
    auto const handle_height = geom::DeltaY{handle_diameter};
    auto const control_stack_offset = geom::DeltaY{handle_diameter + zoom_stack_padding};

    return {
        .drag = {right - handle_width, bottom - handle_height},
        .resize = {left, top},
        .zoom_in = {right - handle_width, top},
        .zoom_out = {right - handle_width, top + control_stack_offset}};
}

auto controls::contains_content(
    miral::magnifier_layout::FreePlacement const& placement,
    double const magnification,
    geom::PointD const& point) -> bool
{
    auto const bounds = miral::magnifier_geometry::visual_bounds(
        placement.surface_top_left, placement.capture_area.size, magnification);

    if (!bounds.contains(point))
        return false;

    // Make sure the point is not in any of the handles, which are drawn on top of the content.
    auto const positions = positions_for(placement, magnification);
    return std::ranges::none_of(
        std::initializer_list{positions.drag, positions.resize, positions.zoom_in, positions.zoom_out},
        [&point](auto const& handle)
        {
            auto const handle_rect =
                geom::RectangleD{geom::PointD{handle}, geom::SizeD{handle_diameter, handle_diameter}};
            return handle_rect.contains(point);
        });
}
