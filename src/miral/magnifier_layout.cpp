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

#include "magnifier_layout.h"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace geom = mir::geometry;

namespace
{
auto constexpr zoom_stack_padding = 8;
geom::Height const zoom_stack_height{
    2 * miral::MagnifierLayout::handle_diameter + zoom_stack_padding};
auto const minimum_visual_dimension =
    2 * miral::MagnifierLayout::handle_diameter + zoom_stack_height.as_int();

auto outputs_usable(geom::Rectangles const& outputs) -> bool
{
    return outputs.size() != 0 &&
           std::ranges::none_of(
               outputs,
               [](auto const& output)
               { return output.size.width == geom::Width{0} || output.size.height == geom::Height{0}; });
}

auto find_current_output(geom::Rectangles const& outputs, geom::Rectangle const& bounds) -> geom::Rectangle
{
    auto maximum_overlap = 0u;
    auto current_output = outputs.begin();

    for (auto output = outputs.begin(); output != outputs.end(); ++output)
    {
        auto const overlap = geom::generic::intersection_of(*output, bounds);
        auto const overlap_area = overlap.size.width.as_uint32_t() * overlap.size.height.as_uint32_t();
        if (overlap_area > maximum_overlap)
        {
            maximum_overlap = overlap_area;
            current_output = output;
        }
    }

    return *current_output;
}

auto overlaps_multiple_outputs(geom::Rectangles const& outputs, geom::Rectangle const& bounds) -> bool
{
    auto overlap_count = 0;
    return std::ranges::any_of(
        outputs,
        [&](auto const& output)
        { return output.overlaps(bounds) && ++overlap_count > 1; });
}

auto minimum_clamped(geom::Size size) -> geom::Size
{
    return {
        std::max(size.width, geom::Width{minimum_visual_dimension}),
        std::max(size.height, geom::Height{minimum_visual_dimension})};
}

auto output_clamped(geom::Size size, geom::Size output_size) -> geom::Size
{
    auto const maximum_size = output_size * 0.8;
    return {
        std::min(size.width, maximum_size.width),
        std::min(size.height, maximum_size.height)};
}

auto center(geom::Point top_left, geom::Size size) -> geom::Point
{
    return top_left +
           geom::Displacement{
               geom::as_delta(size.width / 2),
               geom::as_delta(size.height / 2)};
}

auto top_left_centered_on(geom::Point center_point, geom::Size size) -> geom::Point
{
    return center_point -
           geom::Displacement{
               geom::as_delta(size.width / 2),
               geom::as_delta(size.height / 2)};
}
}

miral::MagnifierLayout::MagnifierLayout(
    geom::Rectangles const& outputs,
    geom::Size requested_visual_size,
    float magnification) :
    outputs{outputs},
    requested_visual_size{minimum_clamped(requested_visual_size)},
    magnification{magnification}
{
}

auto miral::MagnifierLayout::centered_on(geom::Point center_point) const -> Placement
{
    auto const candidate_logical_size = logical_size(requested_visual_size);
    auto const candidate_top_left = top_left_centered_on(center_point, candidate_logical_size);
    auto const visual_size = effective_visual_size(candidate_top_left);
    auto const surface_top_left = top_left_centered_on(center_point, logical_size(visual_size));
    return placement_with_size(surface_top_left, surface_top_left, visual_size);
}

auto miral::MagnifierLayout::freely_positioned(geom::Point desired_surface_top_left) const -> Placement
{
    return freely_positioned_with_size(
        desired_surface_top_left,
        effective_visual_size(desired_surface_top_left));
}

auto miral::MagnifierLayout::freely_positioned_centered_on(geom::Point center_point) const -> Placement
{
    auto const candidate_logical_size = logical_size(requested_visual_size);
    auto const candidate_top_left = top_left_centered_on(center_point, candidate_logical_size);
    auto const visual_size = effective_visual_size(candidate_top_left);
    return freely_positioned_with_size(
        top_left_centered_on(center_point, logical_size(visual_size)),
        visual_size);
}

auto miral::MagnifierLayout::resized_from_pinned_corner(
    geom::Point pinned_visual_bottom_right,
    geom::Point dragged_visual_top_left) const -> Placement
{
    auto const raw_visual_size = minimum_clamped({
        geom::as_width(pinned_visual_bottom_right.x - dragged_visual_top_left.x),
        geom::as_height(pinned_visual_bottom_right.y - dragged_visual_top_left.y)});
    auto const to_logical_dimension = [this](auto visual_dimension)
    {
        return decltype(visual_dimension){
            std::min(static_cast<int>(std::ceil(visual_dimension.as_value() / magnification)), 1000)};
    };
    auto const resized_logical_size = geom::Size{
        to_logical_dimension(raw_visual_size.width),
        to_logical_dimension(raw_visual_size.height)};
    auto const resized_visual_size = resized_logical_size * magnification;
    auto const outer = (magnification + 1.0f) / 2.0f;
    auto const desired_surface_top_left =
        pinned_visual_bottom_right -
        geom::Displacement{
            geom::as_delta(outer * resized_logical_size.width),
            geom::as_delta(outer * resized_logical_size.height)};

    // Feed the resized footprint through the same output clamping and capture
    // mapping as any other freely positioned placement.
    return MagnifierLayout{outputs, resized_visual_size, magnification}.freely_positioned(desired_surface_top_left);
}

auto miral::MagnifierLayout::handle_positions(Placement const& placement) const -> HandlePositions
{
    auto const bounds = visual_bounds(placement);
    auto const zoom_x = bounds.right() - geom::DeltaX{handle_diameter};
    auto const zoom_y =
        bounds.top() + geom::as_delta(bounds.size.height / 2) - geom::as_delta(zoom_stack_height / 2);

    return {
        .drag = {
            bounds.right() - geom::DeltaX{handle_diameter},
            bounds.bottom() - geom::DeltaY{handle_diameter}},
        .resize = bounds.top_left,
        .zoom_in = {zoom_x, zoom_y},
        .zoom_out = {zoom_x, zoom_y + geom::DeltaY{handle_diameter + zoom_stack_padding}}};
}

auto miral::MagnifierLayout::contains_content(
    geom::PointF const& point,
    Placement const& placement) const -> bool
{
    auto const contains = [&point](geom::Rectangle const& area)
    {
        return point.x.as_value() >= area.left().as_value() &&
               point.x.as_value() < area.right().as_value() &&
               point.y.as_value() >= area.top().as_value() &&
               point.y.as_value() < area.bottom().as_value();
    };

    if (!contains(visual_bounds(placement)))
        return false;

    auto const handles = handle_positions(placement);
    for (auto const& handle : {handles.drag, handles.resize, handles.zoom_in, handles.zoom_out})
    {
        if (contains({handle, {handle_diameter, handle_diameter}}))
            return false;
    }

    return true;
}

auto miral::MagnifierLayout::resize_anchor(Placement const& placement) const -> geom::Point
{
    auto const bounds = visual_bounds(placement);
    return {bounds.right(), bounds.bottom()};
}

auto miral::MagnifierLayout::surface_center(Placement const& placement) const -> geom::Point
{
    return center(placement.surface_top_left, placement.capture_area.size);
}

auto miral::MagnifierLayout::capture_center(Placement const& placement) const -> geom::Point
{
    return center(placement.capture_area.top_left, placement.capture_area.size);
}

auto miral::MagnifierLayout::placement_with_size(
    geom::Point capture_top_left,
    geom::Point surface_top_left,
    geom::Size visual_size) const -> Placement
{
    return {
        .capture_area = {capture_top_left, logical_size(visual_size)},
        .surface_top_left = surface_top_left,
        .visual_size = visual_size};
}

auto miral::MagnifierLayout::freely_positioned_with_size(
    geom::Point desired_surface_top_left,
    geom::Size visual_size) const -> Placement
{
    auto const logical_size = this->logical_size(visual_size);
    auto const surface_top_left = clamp_surface_position(desired_surface_top_left, logical_size);
    return placement_with_size(
        capture_position_for(surface_top_left, logical_size),
        surface_top_left,
        visual_size);
}

auto miral::MagnifierLayout::logical_size(geom::Size visual_size) const -> geom::Size
{
    return {
        geom::Width{std::max(1.0f, std::round(visual_size.width.as_value() / magnification))},
        geom::Height{std::max(1.0f, std::round(visual_size.height.as_value() / magnification))}};
}

auto miral::MagnifierLayout::effective_visual_size(geom::Point candidate_surface_top_left) const -> geom::Size
{
    if (!outputs_usable(outputs))
        return requested_visual_size;

    auto const candidate = placement_with_size(
        candidate_surface_top_left,
        candidate_surface_top_left,
        requested_visual_size);
    auto const current_output = find_current_output(outputs, visual_bounds(candidate));
    return output_clamped(requested_visual_size, current_output.size);
}

auto miral::MagnifierLayout::visual_bounds(Placement const& placement) const -> geom::Rectangle
{
    auto const logical_size = placement.capture_area.size;
    auto const visual_size = logical_size * magnification;
    auto const top_left =
        placement.surface_top_left -
        geom::Displacement{
            geom::as_delta((magnification - 1.0f) / 2.0f * logical_size.width),
            geom::as_delta((magnification - 1.0f) / 2.0f * logical_size.height)};
    return {top_left, visual_size};
}

auto miral::MagnifierLayout::clamp_surface_position(
    geom::Point desired_surface_top_left,
    geom::Size logical_size) const -> geom::Point
{
    if (!outputs_usable(outputs))
        return desired_surface_top_left;

    auto const placement = placement_with_size(
        desired_surface_top_left,
        desired_surface_top_left,
        logical_size * magnification);
    auto const bounds = visual_bounds(placement);
    if (overlaps_multiple_outputs(outputs, bounds))
        return desired_surface_top_left;

    auto const current_output = find_current_output(outputs, bounds);
    auto const output_right = current_output.right();
    auto const output_bottom = current_output.bottom();
    auto dx = geom::DeltaX{0};
    if (bounds.left() < current_output.left())
        dx = current_output.left() - bounds.left();
    else if (bounds.right() > output_right)
        dx = output_right - bounds.right();

    auto dy = geom::DeltaY{0};
    if (bounds.top() < current_output.top())
        dy = current_output.top() - bounds.top();
    else if (bounds.bottom() > output_bottom)
        dy = output_bottom - bounds.bottom();

    return desired_surface_top_left + geom::Displacement{dx, dy};
}

auto miral::MagnifierLayout::capture_position_for(
    geom::Point surface_top_left,
    geom::Size logical_size) const -> geom::Point
{
    if (!outputs_usable(outputs))
        return surface_top_left;

    auto const placement =
        placement_with_size(surface_top_left, surface_top_left, logical_size * magnification);
    auto const bounds = visual_bounds(placement);
    auto const current_output = find_current_output(outputs, bounds);
    auto const handle_clearance = static_cast<int>(std::ceil(handle_diameter / magnification));

    struct AxisMapping
    {
        int visual_start;
        int visual_extent;
        int logical_extent;
        int output_start;
        int output_extent;
    };

    auto const map_axis = [handle_clearance](AxisMapping const& axis)
    {
        auto const visual_travel = axis.output_extent - axis.visual_extent;
        if (visual_travel <= 0)
            return axis.output_start + (axis.output_extent - axis.logical_extent) / 2;

        auto const progress =
            std::clamp(static_cast<double>(axis.visual_start - axis.output_start) / visual_travel, 0.0, 1.0);
        auto const out_of_bounds = std::min(axis.logical_extent / 2, handle_clearance);
        auto const capture_start = axis.output_start - out_of_bounds;
        auto const capture_travel = axis.output_extent - axis.logical_extent + 2 * out_of_bounds;
        return capture_start + static_cast<int>(std::round(progress * capture_travel));
    };

    return {
        map_axis({
            .visual_start = bounds.left().as_value(),
            .visual_extent = bounds.size.width.as_value(),
            .logical_extent = logical_size.width.as_value(),
            .output_start = current_output.left().as_value(),
            .output_extent = current_output.size.width.as_value()}),
        map_axis({
            .visual_start = bounds.top().as_value(),
            .visual_extent = bounds.size.height.as_value(),
            .logical_extent = logical_size.height.as_value(),
            .output_start = current_output.top().as_value(),
            .output_extent = current_output.size.height.as_value()})};
}
