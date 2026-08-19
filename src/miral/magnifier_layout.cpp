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
#include <limits>
#include <optional>
#include <ranges>
#include <utility>

namespace geom = mir::geometry;

// Definitions are ordered so that helpers precede their callers: free
// functions, then Placement, then MagnifierLayout's private helpers, then its
// public entry points.

namespace
{
auto constexpr zoom_stack_padding{8};
geom::Height const zoom_stack_height{2 * miral::MagnifierLayout::handle_diameter + zoom_stack_padding};
/// The magnifier must stay wide enough for the resize handle and the zoom
/// stack to sit side by side along the top edge, and tall enough for the zoom
/// stack to clear the drag handle in the bottom right.
geom::Width const minimum_visual_width{2 * miral::MagnifierLayout::handle_diameter};
geom::Height const minimum_visual_height{
    zoom_stack_height + geom::DeltaY{miral::MagnifierLayout::handle_diameter}};
auto constexpr maximum_capture_dimension{1000};

/// The integral top-left offset between a capture rectangle and its bounds
/// after scaling about the centre. The opposite offset may differ by one pixel.
auto centre_inset_of(geom::Size capture_size, float magnification) -> geom::Displacement
{
    auto const inset = (magnification - 1.0f) / 2.0f;
    return {geom::as_delta(inset * capture_size.width), geom::as_delta(inset * capture_size.height)};
}

auto scale_around_centre(geom::Point surface_top_left, geom::Size capture_size, float magnification)
    -> geom::Rectangle
{
    return {surface_top_left - centre_inset_of(capture_size, magnification), capture_size * magnification};
}

/// Inverse of scale_around_centre(), solving for the surface origin that puts the
/// visible rectangle's top left where the caller wants it.
///
/// Both directions must derive the inset from the same integral capture size.
/// Reconstructing it from the visual size with reciprocal magnification rounds
/// the size and inset independently, causing magnification-dependent drift.
auto shrink_point_around_centre(
    geom::Point visual_top_left, geom::Size capture_size, float magnification) -> geom::Point
{
    return visual_top_left + centre_inset_of(capture_size, magnification);
}

auto top_left_centered_on(geom::Point center_point, geom::Size size) -> geom::Point
{
    return center_point - geom::Displacement{geom::as_delta(size.width / 2), geom::as_delta(size.height / 2)};
}

auto clamp_to_minimum(geom::Size size) -> geom::Size
{
    return {std::max(size.width, minimum_visual_width), std::max(size.height, minimum_visual_height)};
}

auto clamp_to_output(geom::Size size, geom::Size output_size) -> geom::Size
{
    auto const maximum_size = output_size * 0.8;
    return {std::min(size.width, maximum_size.width), std::min(size.height, maximum_size.height)};
}

auto is_usable(geom::Rectangle const& output) -> bool
{
    return output.size.width > geom::Width{0} && output.size.height > geom::Height{0};
}

/// Squared distance between a point and rectangle.
auto squared_distance_between(geom::Point const& p, geom::Rectangle const& r) -> long long
{
    auto const dx = std::max({0, r.left().as_value() - p.x.as_value(), p.x.as_value() - r.right().as_value()});
    auto const dy = std::max({0, r.top().as_value() - p.y.as_value(), p.y.as_value() - r.bottom().as_value()});
    return static_cast<long long>(dx) * dx + static_cast<long long>(dy) * dy;
}

/// The output the magnifier is considered to be on is either the one
/// containing its center, or the closest one to its center. Callers only use
/// the result to clamp, and the nearest output is the one the magnifier will
/// be pulled back onto.
auto current_output(geom::Rectangles const& outputs, geom::Rectangle const& bounds)
    -> std::optional<geom::Rectangle>
{
    auto usable = outputs | std::views::filter(is_usable);
    if (std::ranges::empty(usable))
        return std::nullopt;

    // Find the output containing the magnifier's center, if any.
    auto const bounds_center = bounds.centre();
    auto const output_containing_center =
        std::ranges::find_if(usable, [bounds_center](auto const& output) { return output.contains(bounds_center); });
    if (output_containing_center != std::ranges::end(usable))
        return *output_containing_center;

    // Find the closest output to the magnifier's center
    auto current = *std::ranges::begin(usable);
    auto current_distance = squared_distance_between(bounds_center, current);
    for (auto const& output : usable)
    {
        auto const distance = squared_distance_between(bounds_center, output);
        if (distance < current_distance)
        {
            current = output;
            current_distance = distance;
        }
    }

    return current;
}

/// True when every corner of `bounds` lies on some output. A magnifier
/// straddling the shared edge of two outputs satisfies this and is left alone;
/// one hanging off the desktop does not, and is pulled back.
///
/// Corners only: gapped, C-shaped, or U-shaped layouts can have all four
/// corners covered while some of `bounds` sits over empty space. Confining such a
/// placement would be no better - there is no single output it belongs to -
/// so the cheap test is the useful one.
auto is_fully_on_outputs(geom::Rectangles const& outputs, geom::Rectangle const& bounds) -> bool
{
    auto const covered = [&outputs](geom::Point point)
    {
        return std::ranges::any_of(outputs, [point](auto const& output) { return output.contains(point); });
    };

    auto const right = bounds.right() - geom::DeltaX{1};
    auto const bottom = bounds.bottom() - geom::DeltaY{1};

    return covered(bounds.top_left) && covered({right, bounds.top()}) && covered({bounds.left(), bottom}) &&
           covered({right, bottom});
}

auto as_rectangle_f(geom::Rectangle const& rect) -> geom::RectangleF
{
    return {
        {geom::XF{rect.left().as_value()}, geom::YF{rect.top().as_value()}},
        {geom::WidthF{rect.size.width.as_value()}, geom::HeightF{rect.size.height.as_value()}}};
}
}

auto miral::MagnifierLayout::HandlePositions::for_kind(HandleKind kind) const -> geom::Point
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

auto miral::MagnifierLayout::Placement::visual_bounds() const -> geom::Rectangle
{
    return scale_around_centre(untransformed_surface_top_left, capture_area.size, magnification);
}

auto miral::MagnifierLayout::Placement::scaling_center() const -> geom::Point
{
    return geom::Rectangle{untransformed_surface_top_left, capture_area.size}.centre();
}

auto miral::MagnifierLayout::Placement::resize_anchor() const -> geom::Point
{
    auto const bounds = visual_bounds();
    return {bounds.right(), bounds.bottom()};
}

auto miral::MagnifierLayout::Placement::handle_positions() const -> HandlePositions
{
    auto const bounds = visual_bounds();
    auto const zoom_x = bounds.right() - geom::DeltaX{handle_diameter};

    return {
        .drag = {bounds.right() - geom::DeltaX{handle_diameter}, bounds.bottom() - geom::DeltaY{handle_diameter}},
        .resize = bounds.top_left,
        .zoom_in = {zoom_x, bounds.top()},
        .zoom_out = {zoom_x, bounds.top() + geom::DeltaY{handle_diameter + zoom_stack_padding}}};
}

auto miral::MagnifierLayout::Placement::contains_content(geom::PointF const& point) const -> bool
{
    if (!as_rectangle_f(visual_bounds()).contains(point))
        return false;

    // Scene stacking does not save us here: the consumer hit-tests this from
    // an input filter prepended to the composite event filter, which runs
    // before the handle surfaces are offered the event. Reporting content
    // under a handle would let the magnifier swallow that handle's clicks.
    auto const handles = handle_positions();
    auto const covers = [&point](geom::Point handle)
    {
        return as_rectangle_f({handle, {handle_diameter, handle_diameter}}).contains(point);
    };

    return std::ranges::none_of(
        std::initializer_list{handles.drag, handles.resize, handles.zoom_in, handles.zoom_out},
        [&covers](auto const& handle) { return covers(handle); });
}

miral::MagnifierLayout::MagnifierLayout(
    geom::Rectangles const& outputs, geom::Size preferred_visual_size, float magnification) :
    outputs{outputs},
    preferred_visual_size{clamp_to_minimum(preferred_visual_size)},
    magnification{magnification}
{
}

auto miral::MagnifierLayout::capture_size_for(geom::Size visual_size) const -> geom::Size
{
    return visual_size / magnification;
}

auto miral::MagnifierLayout::clamped_visual_size_at(
    geom::Point surface_top_left, geom::Size visual_size) const -> geom::Size
{
    auto const output =
        current_output(outputs, scale_around_centre(surface_top_left, capture_size_for(visual_size), magnification));

    return output ? clamp_to_output(visual_size, output->size) : visual_size;
}

auto miral::MagnifierLayout::confined_surface_position(
    geom::Point surface_top_left, geom::Size capture_size) const -> geom::Point
{
    auto const bounds = scale_around_centre(surface_top_left, capture_size, magnification);
    auto const output = current_output(outputs, bounds);

    // A magnifier resting entirely on the desktop is left where the user put
    // it, even when that means straddling two outputs.
    if (!output || is_fully_on_outputs(outputs, bounds))
        return surface_top_left;

    auto dx = geom::DeltaX{0};
    if (bounds.left() < output->left())
        dx = output->left() - bounds.left();
    else if (bounds.right() > output->right())
        dx = output->right() - bounds.right();

    auto dy = geom::DeltaY{0};
    if (bounds.top() < output->top())
        dy = output->top() - bounds.top();
    else if (bounds.bottom() > output->bottom())
        dy = output->bottom() - bounds.bottom();

    return surface_top_left + geom::Displacement{dx, dy};
}

auto miral::MagnifierLayout::capture_position_for(
    geom::Point surface_top_left, geom::Size capture_size) const -> geom::Point
{
    auto const bounds = scale_around_centre(surface_top_left, capture_size, magnification);
    auto const output = current_output(outputs, bounds);
    if (!output)
        return surface_top_left;

    // Map the magnifier's travel across the output onto the capture's travel
    // across that same output, so that pushing the magnifier to an edge
    // captures that edge. The capture is allowed to run slightly off the
    // output (by the width of a handle) so that content underneath the
    // controls remains reachable.
    auto const handle_clearance = static_cast<int>(std::ceil(handle_diameter / magnification));

    struct Axis
    {
        int visual_start;
        int visual_extent;
        int capture_extent;
        int output_start;
        int output_extent;
    };

    // One axis at a time. Two boxes slide along it: the magnifier over the
    // output, and the capture over a track stretched past the output by
    // out_of_bounds at either end, so that the content hidden beneath the edge
    // handles can still be captured. `progress` is how far along its own track
    // the magnifier sits, and the capture is put the same fraction along its
    // own. That pins the two tracks' extremes together, which is the whole
    // point: shove the magnifier against an output edge and that is the edge
    // that gets captured.
    //
    // Below, both boxes are drawn at progress 0 and again at progress 1, to a
    // shared scale, so one column of the figure is one coordinate on the axis:
    //
    //                     output_start          output_start + output_extent
    //                     v                                                v
    // output              ==================================================
    // magnifier           {visual_extent}                  {visual_extent}
    //                     |<------- visual_travel -------->|
    // capture       {capture_extent}                                {capture_extent}
    //               |<---------- capture_travel ------------------->|
    //               ^
    //               capture_start = output_start - out_of_bounds
    //
    // Each arrow spans the positions that box's *start* can take, so the code
    // below is just the figure read downwards: measure progress along the
    // upper arrow, then step the same fraction along the lower one.
    auto const map_axis = [handle_clearance](Axis const& axis)
    {
        auto const visual_travel = axis.output_extent - axis.visual_extent;
        auto const progress =
            std::clamp(static_cast<double>(axis.visual_start - axis.output_start) / visual_travel, 0.0, 1.0);
        auto const out_of_bounds = std::min(axis.capture_extent / 2, handle_clearance);
        auto const capture_start = axis.output_start - out_of_bounds;
        auto const capture_travel = axis.output_extent - axis.capture_extent + 2 * out_of_bounds;
        return capture_start + static_cast<int>(std::round(progress * capture_travel));
    };

    return {
        map_axis({
            .visual_start = bounds.left().as_value(),
            .visual_extent = bounds.size.width.as_value(),
            .capture_extent = capture_size.width.as_value(),
            .output_start = output->left().as_value(),
            .output_extent = output->size.width.as_value()}),
        map_axis({
            .visual_start = bounds.top().as_value(),
            .visual_extent = bounds.size.height.as_value(),
            .capture_extent = capture_size.height.as_value(),
            .output_start = output->top().as_value(),
            .output_extent = output->size.height.as_value()})};
}

auto miral::MagnifierLayout::place_freely_at_visual(
    geom::Point visual_top_left, geom::Size preferred) const -> Placement
{
    // Sizing depends on which output the magnifier lands on, which depends on
    // where it is placed, so choose an output from a provisional placement and
    // then honour the requested visual corner using the size that results.
    auto const provisional = shrink_point_around_centre(
        visual_top_left, capture_size_for(preferred), magnification);
    auto const capture_size = capture_size_for(clamped_visual_size_at(provisional, preferred));

    auto const surface_top_left =
        shrink_point_around_centre(visual_top_left, capture_size, magnification);
    auto const confined = confined_surface_position(surface_top_left, capture_size);

    return {
        .capture_area = {capture_position_for(confined, capture_size), capture_size},
        .untransformed_surface_top_left = confined,
        .preferred_visual_size = preferred,
        .magnification = magnification};
}

auto miral::MagnifierLayout::placement_of(
    geom::Rectangle capture_area, geom::Point untransformed_surface_top_left) const -> Placement
{
    return {
        .capture_area = capture_area,
        .untransformed_surface_top_left = untransformed_surface_top_left,
        .preferred_visual_size = preferred_visual_size,
        .magnification = magnification};
}

auto miral::MagnifierLayout::place_following_cursor_at(geom::Point cursor) const -> Placement
{
    auto const provisional = top_left_centered_on(cursor, capture_size_for(preferred_visual_size));
    auto const capture_size =
        capture_size_for(clamped_visual_size_at(provisional, preferred_visual_size));
    auto const surface_top_left = top_left_centered_on(cursor, capture_size);

    // Follow-cursor placement captures exactly the region the surface covers,
    // and is deliberately left unconfined so the magnifier can track the
    // cursor into a screen corner.
    return {
        .capture_area = {surface_top_left, capture_size},
        .untransformed_surface_top_left = surface_top_left,
        .preferred_visual_size = preferred_visual_size,
        .magnification = magnification};
}

auto miral::MagnifierLayout::place_freely_at(geom::Point visual_top_left) const -> Placement
{
    return place_freely_at_visual(visual_top_left, preferred_visual_size);
}

auto miral::MagnifierLayout::place_freely_centered_on(geom::Point center_point) const -> Placement
{
    auto const provisional = top_left_centered_on(center_point, capture_size_for(preferred_visual_size));
    auto const capture_size =
        capture_size_for(clamped_visual_size_at(provisional, preferred_visual_size));

    return place_freely_at_visual(
        scale_around_centre(top_left_centered_on(center_point, capture_size), capture_size, magnification)
            .top_left,
        preferred_visual_size);
}

auto miral::MagnifierLayout::resize_from_pinned_corner(
    geom::Point pinned_visual_bottom_right, geom::Point dragged_visual_top_left) const -> Placement
{
    auto const dragged_visual_size = clamp_to_minimum(
        {geom::as_width(pinned_visual_bottom_right.x - dragged_visual_top_left.x),
         geom::as_height(pinned_visual_bottom_right.y - dragged_visual_top_left.y)});

    auto const capped_capture_dimension = [this](auto visual_dimension)
    {
        return decltype(visual_dimension){std::min(
            static_cast<int>(std::ceil(static_cast<float>(visual_dimension.as_value()) / magnification)),
            maximum_capture_dimension)};
    };
    auto const capture_size = geom::Size{
        capped_capture_dimension(dragged_visual_size.width),
        capped_capture_dimension(dragged_visual_size.height)};

    // Holding the bottom right corner still simply means putting the top left
    // corner a full visual extent above and to the left of it. Derive that
    // extent from the size placement will actually settle on, so the pinned
    // corner does not drift by a pixel as the capture size is rounded.
    auto const preferred = clamp_to_minimum(capture_size * magnification);
    auto const unclamped_footprint = capture_size_for(preferred) * magnification;
    auto const provisional = shrink_point_around_centre(
        pinned_visual_bottom_right - as_displacement(unclamped_footprint), capture_size_for(preferred), magnification);
    auto const footprint = capture_size_for(clamped_visual_size_at(provisional, preferred)) * magnification;

    return place_freely_at_visual(
        pinned_visual_bottom_right - as_displacement(footprint), preferred);
}
