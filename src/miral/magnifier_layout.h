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

#ifndef MIRAL_MAGNIFIER_LAYOUT_H
#define MIRAL_MAGNIFIER_LAYOUT_H

#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/rectangles.h>
#include <mir/geometry/size.h>

namespace miral::magnifier_layout
{
struct Placement
{
    mir::geometry::Rectangle capture_area;

    bool operator==(Placement const&) const = default;
};

struct FreePlacement : Placement
{
    /// Top-left of the untransformed surface. Its size is always
    /// `capture_area.size`.
    mir::geometry::Point surface_top_left;

    bool operator==(FreePlacement const&) const = default;
};

auto place_following_cursor(
    mir::geometry::PointD cursor,
    mir::geometry::SizeD requested_visual_size,
    mir::geometry::Rectangles const& outputs,
    double magnification) -> Placement;

auto place_freely(
    mir::geometry::PointD desired_center,
    mir::geometry::SizeD requested_visual_size,
    mir::geometry::Rectangles const& outputs,
    double magnification) -> FreePlacement;

auto resize_freely(
    mir::geometry::PointD dragged_visual_top_left,
    mir::geometry::PointD pinned_visual_bottom_right,
    mir::geometry::Rectangles const& outputs,
    double magnification) -> FreePlacement;
}

#endif
