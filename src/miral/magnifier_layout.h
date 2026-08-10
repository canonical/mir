/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_MAGNIFIER_LAYOUT_H
#define MIRAL_MAGNIFIER_LAYOUT_H

#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/rectangles.h>
#include <mir/geometry/size.h>

namespace miral
{
class MagnifierLayout
{
public:
    static int constexpr handle_diameter = 48;

    struct Placement
    {
        // capture_area.size is the logical size corresponding to visual_size
        // at this layout's magnification.
        mir::geometry::Rectangle capture_area;
        mir::geometry::Point surface_top_left;
        mir::geometry::Size visual_size;
    };

    struct HandlePositions
    {
        mir::geometry::Point drag;
        mir::geometry::Point resize;
        mir::geometry::Point zoom_in;
        mir::geometry::Point zoom_out;
    };

    MagnifierLayout(
        mir::geometry::Rectangles const& outputs,
        mir::geometry::Size requested_visual_size,
        float magnification);

    auto centered_on(mir::geometry::Point center) const -> Placement;
    auto freely_positioned(mir::geometry::Point desired_surface_top_left) const -> Placement;
    auto freely_positioned_centered_on(mir::geometry::Point center) const -> Placement;
    auto resized_from_pinned_corner(
        mir::geometry::Point pinned_visual_bottom_right,
        mir::geometry::Point dragged_visual_top_left) const -> Placement;

    auto handle_positions(Placement const& placement) const -> HandlePositions;
    auto contains_content(mir::geometry::PointF const& point, Placement const& placement) const -> bool;
    auto resize_anchor(Placement const& placement) const -> mir::geometry::Point;
    auto surface_center(Placement const& placement) const -> mir::geometry::Point;
    auto capture_center(Placement const& placement) const -> mir::geometry::Point;

private:
    auto placement_with_size(
        mir::geometry::Point capture_top_left,
        mir::geometry::Point surface_top_left,
        mir::geometry::Size visual_size) const -> Placement;
    auto freely_positioned_with_size(
        mir::geometry::Point desired_surface_top_left,
        mir::geometry::Size visual_size) const -> Placement;
    auto logical_size(mir::geometry::Size visual_size) const -> mir::geometry::Size;
    auto effective_visual_size(mir::geometry::Point candidate_surface_top_left) const -> mir::geometry::Size;
    auto visual_bounds(Placement const& placement) const -> mir::geometry::Rectangle;
    auto clamp_surface_position(
        mir::geometry::Point desired_surface_top_left,
        mir::geometry::Size logical_size) const -> mir::geometry::Point;
    auto capture_position_for(
        mir::geometry::Point surface_top_left,
        mir::geometry::Size logical_size) const -> mir::geometry::Point;

    mir::geometry::Rectangles outputs;
    mir::geometry::Size requested_visual_size;
    float magnification;
};
}

#endif
