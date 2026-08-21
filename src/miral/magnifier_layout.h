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

/// Geometry of the magnifier.
///
/// Three rectangles are in play, all in the same global screen coordinates.
/// Telling them apart is the key to reading this file:
///
///   1. capture area  - the region read out of the scene. Small.
///   2. surface rect  - where the surface is placed before the compositor
///                      scales it. Same size as the capture area.
///   3. visual bounds - what the user actually sees: the surface scaled by
///                      `magnification` **about its own centre**.
///
/// Because the scale is about the centre, the surface rect is *inset* within
/// the visual bounds; the surface top left is NOT the corner the user sees:
///
///     visual .-----------------------------.
///            |  inset = (mag-1)/2 * size    |
///            |   surface .-----------.      |
///            |           |  capture  |      |
///            |           |   sized   |      |
///            |           '-----------'      |
///            |                              |
///            '-----------------------------' <- resize_anchor()
///
/// Hence, for a capture area of `size`:
///
///     visual size      = size * magnification
///     visual top left  = surface top left - (magnification - 1) / 2 * size
///     surface top left = visual bottom right - (magnification + 1) / 2 * size
///
/// Everything the user points at - handles, hit testing, dragging and resizing
/// - is expressed in **visual** coordinates.
/// `Placement::untransformed_surface_top_left` is an output only: it exists to
/// be handed to `Surface::move_to()`.
///
/// Three sizes are likewise distinct, and conflating them is a bug:
///
///   * preferred visual size - persistent user intent. Only ever constrained
///     by permanent limits (the minimum that fits the controls). Never
///     narrowed by whichever output the magnifier happens to sit on, so that
///     moving to a small output and back again is lossless.
///   * clamped visual size   - the preferred size cut down to fit the current
///     output. Transient; recomputed for every placement.
///   * actual footprint      - `capture_area.size * magnification`, i.e. the
///     clamped size after the capture size was rounded to whole pixels. This
///     is what `visual_bounds()` reports.
namespace miral
{
/// Identifies a magnifier control handle.
enum class HandleKind { drag, resize, zoom_in, zoom_out };

class MagnifierLayout
{
public:
    static int constexpr handle_diameter = 48;

    /// Top left of each control handle, in visual coordinates. Every handle is
    /// `handle_diameter` square.
    struct HandlePositions
    {
        mir::geometry::Point drag;
        mir::geometry::Point resize;
        mir::geometry::Point zoom_in;
        mir::geometry::Point zoom_out;

        auto for_kind(HandleKind kind) const -> mir::geometry::Point;

        bool operator==(HandlePositions const&) const = default;
    };

    /// A resolved placement. Self-describing: everything derivable from a
    /// placement hangs off it, so callers never need the layout that made it.
    struct Placement
    {
        mir::geometry::Rectangle capture_area;
        /// Feed this to `Surface::move_to()`. Per the diagram above, it is not
        /// the corner the user sees.
        mir::geometry::Point untransformed_surface_top_left;
        /// Persistent user intent, never narrowed by output clamping. Callers
        /// should store this and hand it back when rebuilding the layout.
        mir::geometry::Size preferred_visual_size;
        float magnification;

        bool operator==(Placement const&) const = default;

        /// What the user sees: the surface scaled about its centre.
        auto visual_bounds() const -> mir::geometry::Rectangle;
        /// The point magnification scales about, and which is therefore
        /// preserved across a zoom change. Derived from the surface rect:
        /// because the capture size is rounded to whole pixels this can sit a
        /// pixel away from the integer centre of `visual_bounds()`.
        auto scaling_center() const -> mir::geometry::Point;
        /// The visual corner held still while the opposite one is dragged.
        auto resize_anchor() const -> mir::geometry::Point;
        auto handle_positions() const -> HandlePositions;
        /// True when the point is over magnified content rather than a control.
        auto contains_content(mir::geometry::PointF const& point) const -> bool;
    };

    MagnifierLayout(
        mir::geometry::Rectangles const& outputs,
        mir::geometry::Size preferred_visual_size,
        float magnification);

    /// Follow-cursor placement: captures exactly the region it covers, and is
    /// deliberately not confined to an output so that the magnifier can track
    /// the cursor into a screen corner.
    auto place_following_cursor_at(mir::geometry::Point cursor) const -> Placement;

    /// Freely positioned placement: confined to an output, with the captured
    /// region mapped proportionally across that output.
    auto place_freely_at(mir::geometry::Point visual_top_left) const -> Placement;
    auto place_freely_centered_on(mir::geometry::Point center) const -> Placement;

    /// Freely positioned placement resized by dragging one visual corner while
    /// the opposite corner stays put.
    auto resize_from_pinned_corner(
        mir::geometry::Point pinned_visual_bottom_right,
        mir::geometry::Point dragged_visual_top_left) const -> Placement;

    /// Rebuilds a placement from state the caller already holds. The sanctioned
    /// way to obtain a Placement without re-running placement, so that the
    /// derived fields stay consistent with this layout.
    auto placement_of(
        mir::geometry::Rectangle capture_area,
        mir::geometry::Point untransformed_surface_top_left) const -> Placement;

private:
    auto place_freely_at_visual(
        mir::geometry::Point visual_top_left,
        mir::geometry::Size preferred) const -> Placement;
    auto capture_size_for(mir::geometry::Size visual_size) const -> mir::geometry::Size;
    auto clamped_visual_size_at(
        mir::geometry::Point surface_top_left,
        mir::geometry::Size visual_size) const -> mir::geometry::Size;
    auto confined_surface_position(
        mir::geometry::Point surface_top_left,
        mir::geometry::Size capture_size) const -> mir::geometry::Point;
    auto capture_position_for(
        mir::geometry::Point surface_top_left,
        mir::geometry::Size capture_size) const -> mir::geometry::Point;

    mir::geometry::Rectangles outputs;
    mir::geometry::Size preferred_visual_size;
    float magnification;
};
}

#endif
