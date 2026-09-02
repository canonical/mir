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

#include "magnifier_layout.h"

#include "magnifier_controls.h"
#include "magnifier_geometry.h"

#include <algorithm>
#include <cmath>

namespace geom = mir::geometry;
namespace controls = miral::magnifier_controls;
namespace layout = miral::magnifier_layout;
using namespace miral::magnifier_geometry;

namespace
{
template<typename RectangleType>
inline auto as_rectangle_d(RectangleType rectangle) -> mir::geometry::RectangleD
{
    return {
        mir::geometry::PointD{rectangle.top_left},
        mir::geometry::SizeD{rectangle.size},
    };
}

auto squared_distance_between(geom::PointD point, geom::Rectangle rectangle) -> double
{
    auto const rectangle_d = as_rectangle_d(rectangle);
    auto const dx = std::max({
        0.0,
        (rectangle_d.left() - point.x).as_value(),
        (point.x - rectangle_d.right()).as_value(),
    });
    auto const dy = std::max({
        0.0,
        (rectangle_d.top() - point.y).as_value(),
        (point.y - rectangle_d.bottom()).as_value(),
    });
    return dx * dx + dy * dy;
}

auto selected_output(geom::Rectangles const& outputs, geom::PointD desired_center)
    -> geom::Rectangle const&
{
    auto const first_containing = std::ranges::find_if(
        outputs, [desired_center](auto const& output) { return as_rectangle_d(output).contains(desired_center); });

    if (first_containing != outputs.end())
        return *first_containing;

    auto const nearest = std::ranges::min_element(
        outputs,
        std::less{},
        [desired_center](auto const& output) { return squared_distance_between(desired_center, output); });

    return *nearest;
}

auto clamp(geom::SizeD value, geom::SizeD min_value, geom::SizeD max_value) -> geom::SizeD
{
    return {
        std::clamp(value.width, min_value.width, max_value.width),
        std::clamp(value.height, min_value.height, max_value.height),
    };
}

auto floor(geom::SizeD value) -> geom::Size
{
    return {
        geom::Width{std::floor(value.width.as_value())},
        geom::Height{std::floor(value.height.as_value())}};
}

auto capture_size_for(
    geom::SizeD requested_visual_size,
    geom::Rectangle output,
    double magnification) -> geom::Size
{
    auto const maximum_visual = 0.8 * geom::SizeD{output.size};

    auto const clamped_visual = clamp(requested_visual_size, geom::SizeD{controls::minimum_visual_size}, geom::SizeD{maximum_visual});
    auto const requested_capture = geom::Size{clamped_visual / magnification};
    auto const maximum_capture = floor(maximum_visual / magnification);
    return {
        std::min(requested_capture.width, maximum_capture.width),
        std::min(requested_capture.height, maximum_capture.height)};
}

auto surface_top_left_centered_on(geom::PointD center, geom::Size capture_size) -> geom::Point
{
    return {
        geom::X{center.x.as_value() - capture_size.width.as_value() / 2.0},
        geom::Y{center.y.as_value() - capture_size.height.as_value() / 2.0}};
}

auto contains_point(geom::Rectangles const& outputs, double x, double y) -> bool
{
    return std::ranges::any_of(
        outputs, [point = geom::PointD{x, y}](auto const& output) { return as_rectangle_d(output).contains(point); });
}

auto contains_corners(geom::Rectangles const& outputs, geom::RectangleD const& bounds) -> bool
{
    auto const left = bounds.left().as_value();
    auto const right = bounds.right().as_value();
    auto const top = bounds.top().as_value();
    auto const bottom = bounds.bottom().as_value();
    return contains_point(outputs, left, top) &&
           contains_point(outputs, right, top) &&
           contains_point(outputs, left, bottom) &&
           contains_point(outputs, right, bottom);
}

auto confinement_for(geom::RectangleD const& bounds, geom::Rectangle output) -> geom::Displacement
{
    auto dx = 0;
    auto const output_d = as_rectangle_d(output);
    if (bounds.left() < output_d.left())
        dx = static_cast<int>(std::ceil((output_d.left() - bounds.left()).as_value()));
    else if (bounds.right() > output_d.right())
        dx = static_cast<int>(std::floor((output_d.right() - bounds.right()).as_value()));

    auto dy = 0;
    if (bounds.top() < output_d.top())
        dy = static_cast<int>(std::ceil((output_d.top() - bounds.top()).as_value()));
    else if (bounds.bottom() > output_d.bottom())
        dy = static_cast<int>(std::floor((output_d.bottom() - bounds.bottom()).as_value()));

    return {dx, dy};
}

auto map_axis(
    double clearance,
    double visual_center,
    double visual_extent,
    int capture_extent,
    int output_start,
    int output_extent)
{
    auto const output_center = output_start + output_extent / 2.0;
    auto const progress =
        std::clamp((visual_center - output_center) / ((output_extent - visual_extent) / 2.0), -1.0, 1.0);
    auto const capture_center = output_center + progress * ((output_extent - capture_extent) / 2.0 + clearance);
    return static_cast<int>(capture_center - capture_extent / 2.0);
}

auto capture_top_left_for(
    geom::RectangleD const& visual_bounds,
    geom::Size capture_size,
    geom::Rectangle output,
    double magnification) -> geom::Point
{
    auto const clearance = std::ceil(controls::visual_edge_clearance / magnification);
    auto const visual_center = visual_bounds.centre();
    return {
        map_axis(
            clearance,
            visual_center.x.as_value(),
            visual_bounds.size.width.as_value(),
            capture_size.width.as_value(),
            output.left().as_value(),
            output.size.width.as_value()),
        map_axis(
            clearance,
            visual_center.y.as_value(),
            visual_bounds.size.height.as_value(),
            capture_size.height.as_value(),
            output.top().as_value(),
            output.size.height.as_value())};
}

// Places the magnifier capture area and confines its visual area when necessary.
//
// First, the visual area is calculated from the given surface top left and
// capture size. If the visual area's corners are not all covered by the
// outputs, the surface top left is adjusted to confine it to the selected output.
// Then, the capture area is placed to center on the visual area, while
// ensuring that it does not extend beyond the output more than a handle's diameter.
auto free_placement(
    geom::Rectangle output,
    geom::Size capture_size,
    geom::Point surface_top_left,
    geom::Rectangles const& outputs,
    double magnification) -> layout::FreePlacement
{
    auto bounds = miral::magnifier_geometry::visual_bounds(
        surface_top_left, capture_size, magnification);
    auto confinement = geom::Displacement{0, 0};
    if (!contains_corners(outputs, bounds))
    {
        confinement = confinement_for(bounds, output);
        surface_top_left += confinement;
        bounds.top_left += geom::DisplacementD{confinement};
    }

    return {
        layout::Placement{{capture_top_left_for(bounds, capture_size, output, magnification), capture_size}},
        surface_top_left};
}
}

auto layout::place_following_cursor(
    geom::PointD cursor,
    geom::SizeD requested_visual_size,
    geom::Rectangles const& outputs,
    double magnification) -> Placement
{
    auto const output = selected_output(outputs, cursor);
    auto const capture_size = capture_size_for(requested_visual_size, output, magnification);
    auto const surface_top_left = surface_top_left_centered_on(cursor, capture_size);

    return {
        .capture_area = {surface_top_left, capture_size},
    };
}

auto layout::place_freely(
    geom::PointD desired_center,
    geom::SizeD requested_visual_size,
    geom::Rectangles const& outputs,
    double magnification) -> FreePlacement
{
    auto const output = selected_output(outputs, desired_center);
    auto const capture_size = capture_size_for(requested_visual_size, output, magnification);
    return free_placement(
        output,
        capture_size,
        surface_top_left_centered_on(desired_center, capture_size),
        outputs,
        magnification);
}

auto layout::resize_freely(
    geom::PointD dragged_visual_top_left,
    geom::PointD pinned_visual_bottom_right,
    geom::Rectangles const& outputs,
    double magnification) -> FreePlacement
{
    auto const requested_visual_size = geom::SizeD{
        (pinned_visual_bottom_right.x - dragged_visual_top_left.x).as_value(),
        (pinned_visual_bottom_right.y - dragged_visual_top_left.y).as_value()};

    auto const temp_x = dragged_visual_top_left.x.as_value() + pinned_visual_bottom_right.x.as_value();
    auto const temp_y = dragged_visual_top_left.y.as_value() + pinned_visual_bottom_right.y.as_value();
    auto const intended_center = geom::PointD{(temp_x) / 2.0, (temp_y) / 2.0};

    auto const output = selected_output(outputs, intended_center);
    auto const capture_size = capture_size_for(requested_visual_size, output, magnification);
    auto const surface_top_left = geom::Point{
        geom::X{
            pinned_visual_bottom_right.x.as_value() -
            (1.0 + magnification) * capture_size.width.as_value() / 2.0},
        geom::Y{
            pinned_visual_bottom_right.y.as_value() -
            (1.0 + magnification) * capture_size.height.as_value() / 2.0}};

    return free_placement(output, capture_size, surface_top_left, outputs, magnification);
}
