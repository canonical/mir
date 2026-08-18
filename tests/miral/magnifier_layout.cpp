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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;
using namespace testing;

namespace
{
geom::Rectangles outputs()
{
    geom::Rectangles result;
    result.add({{0, 0}, {800, 600}});
    return result;
}
}

TEST(MagnifierLayout, clamps_visual_size_before_deriving_centered_capture)
{
    miral::MagnifierLayout const layout{outputs(), {1000, 1000}, 2.0f};

    auto const placement = layout.place_following_cursor_at({400, 300});

    EXPECT_THAT(placement.visual_bounds().size, Eq(geom::Size{640, 480}));
    EXPECT_THAT(placement.capture_area, Eq(geom::Rectangle{{240, 180}, {320, 240}}));
    EXPECT_THAT(placement.untransformed_surface_top_left, Eq(placement.capture_area.top_left));
}

TEST(MagnifierLayout, output_clamping_does_not_narrow_the_preferred_size)
{
    miral::MagnifierLayout const layout{outputs(), {1000, 1000}, 2.0f};

    auto const placement = layout.place_following_cursor_at({400, 300});

    // The magnifier is drawn smaller to fit the output, but the size the user
    // asked for survives so that moving back to a larger output restores it.
    EXPECT_THAT(placement.visual_bounds().size, Eq(geom::Size{640, 480}));
    EXPECT_THAT(placement.preferred_visual_size, Eq(geom::Size{1000, 1000}));
}

TEST(MagnifierLayout, freely_positioned_surface_is_clamped_to_output)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};

    auto const placement = layout.place_freely_at({-50, -50});

    EXPECT_THAT(placement.untransformed_surface_top_left, Eq(geom::Point{0, 0}));
}

TEST(MagnifierLayout, clamps_position_when_size_is_unchanged_after_output_removal)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};

    auto const placement = layout.place_freely_at({900, 0});

    EXPECT_THAT(placement.visual_bounds().size, Eq(geom::Size{200, 200}));
    EXPECT_THAT(placement.untransformed_surface_top_left, Eq(geom::Point{600, 0}));
}

TEST(MagnifierLayout, chooses_output_containing_visual_bounds_center)
{
    auto bounds = outputs();
    bounds.add({{800, 0}, {400, 300}});
    miral::MagnifierLayout const layout{bounds, {500, 500}, 2.0f};

    // The visual bounds' center lies on the small output, which determines
    // the size even though the bounds extend farther over the large output.
    auto const placement = layout.place_following_cursor_at({810, 150});

    EXPECT_THAT(placement.visual_bounds().size, Eq(geom::Size{320, 240}));
}

TEST(MagnifierLayout, chooses_nearest_output_when_visual_bounds_center_is_in_a_gap)
{
    geom::Rectangles bounds;
    bounds.add({{900, 0}, {100, 100}});
    bounds.add({{510, 0}, {200, 200}});
    miral::MagnifierLayout const layout{bounds, {1000, 200}, 1.0f};

    // Both outputs overlap the initial visual bounds, but its center is in
    // the gap and much closer to the second output.
    auto const placement = layout.place_following_cursor_at({500, 100});

    EXPECT_THAT(placement.visual_bounds().size, Eq(geom::Size{160, 160}));
}

TEST(MagnifierLayout, follow_cursor_is_not_confined_to_an_output)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};

    // Tracking the cursor into a screen corner deliberately leaves the
    // magnifier hanging off the output rather than losing the cursor.
    auto const placement = layout.place_following_cursor_at({0, 0});

    EXPECT_THAT(placement.visual_bounds().top_left, Eq(geom::Point{-100, -100}));
}

TEST(MagnifierLayout, snaps_back_onto_an_output_when_it_fits_on_none)
{
    auto bounds = outputs();
    bounds.add({{800, 0}, {400, 300}});
    miral::MagnifierLayout const layout{bounds, {500, 500}, 2.0f};

    // The cursor was tracked here while following, and following was then
    // switched off, leaving the magnifier freely positioned where it
    // overhangs the small output on both axes and covers empty space. It is
    // sized for, and pulled back onto, the output it mostly sits on.
    auto const placement = layout.place_freely_centered_on({790, 150});

    EXPECT_THAT(placement.visual_bounds(), Eq(geom::Rectangle{{300, 0}, {500, 480}}));
}

TEST(MagnifierLayout, straddling_two_outputs_is_left_alone)
{
    auto bounds = outputs();
    bounds.add({{800, 0}, {400, 300}});
    miral::MagnifierLayout const layout{bounds, {200, 200}, 1.0f};

    // Spans the shared edge of two outputs of *unequal* height, staying within
    // the shorter one's extent, so every part of it is on the desktop.
    auto const placement = layout.place_freely_at({700, 100});

    EXPECT_THAT(placement.visual_bounds().top_left, Eq(geom::Point{700, 100}));
}

TEST(MagnifierLayout, falls_back_to_the_nearest_output_when_overlapping_none)
{
    auto bounds = outputs();
    bounds.add({{2000, 0}, {400, 300}});
    miral::MagnifierLayout const layout{bounds, {200, 200}, 1.0f};

    // In the gap between the outputs, but nearer the second one.
    auto const placement = layout.place_freely_at({1900, 0});

    EXPECT_THAT(placement.untransformed_surface_top_left, Eq(geom::Point{2000, 0}));
}

TEST(MagnifierLayout, places_the_visible_corner_where_asked)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.25f};

    auto const placement = layout.place_freely_at({100, 100});

    // The surface origin is inset within the visible bounds, so the two differ.
    EXPECT_THAT(placement.visual_bounds().top_left, Eq(geom::Point{100, 100}));
    EXPECT_THAT(placement.untransformed_surface_top_left, Ne(geom::Point{100, 100}));
}

TEST(MagnifierLayout, ignores_outputs_with_no_area)
{
    geom::Rectangles bounds;
    bounds.add({{0, 0}, {0, 0}});
    bounds.add({{0, 0}, {800, 600}});
    miral::MagnifierLayout const layout{bounds, {200, 200}, 1.0f};

    auto const placement = layout.place_freely_at({-50, -50});

    // A single unconfigured output must not disable confinement everywhere.
    EXPECT_THAT(placement.untransformed_surface_top_left, Eq(geom::Point{0, 0}));
}

TEST(MagnifierLayout, stacks_the_zoom_buttons_in_the_top_right_corner)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};
    auto const placement = layout.place_freely_at({100, 100});
    auto const bounds = placement.visual_bounds();

    auto const handles = placement.handle_positions();

    EXPECT_THAT(handles.zoom_in, Eq(geom::Point{bounds.right() - geom::DeltaX{48}, bounds.top()}));
    EXPECT_THAT(handles.zoom_out.x, Eq(handles.zoom_in.x));
    EXPECT_THAT(handles.zoom_out.y - handles.zoom_in.y, Eq(geom::DeltaY{56}));
}

TEST(MagnifierLayout, content_hit_testing_excludes_controls)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};
    auto const placement = layout.place_freely_at({100, 100});
    auto const handles = placement.handle_positions();

    EXPECT_TRUE(placement.contains_content({200.0f, 200.0f}));
    EXPECT_FALSE(placement.contains_content({99.0f, 200.0f}));
    EXPECT_FALSE(placement.contains_content({300.0f, 200.0f}));

    // Every handle must be reported as a control, or the consumer's input
    // filter swallows its clicks before the handle surface is offered them.
    for (auto const kind :
         {miral::HandleKind::drag, miral::HandleKind::resize, miral::HandleKind::zoom_in,
          miral::HandleKind::zoom_out})
    {
        auto const handle = handles.for_kind(kind);
        auto const center = static_cast<float>(miral::MagnifierLayout::handle_diameter) / 2.0f;
        EXPECT_FALSE(placement.contains_content(
            {static_cast<float>(handle.x.as_value()) + center,
             static_cast<float>(handle.y.as_value()) + center}))
            << "handle " << static_cast<int>(kind);
    }
}

TEST(MagnifierLayout, pinned_resize_preserves_opposite_visual_corner)
{
    miral::MagnifierLayout const layout{{}, {200, 200}, 1.25f};
    auto const initial = layout.place_freely_at({100, 100});
    auto const anchor = initial.resize_anchor();

    auto const resized = layout.resize_from_pinned_corner(
        anchor, initial.handle_positions().resize - geom::Displacement{20, 20});

    EXPECT_THAT(resized.resize_anchor(), Eq(anchor));
    EXPECT_THAT(resized.visual_bounds().size.width, Gt(initial.visual_bounds().size.width));
    EXPECT_THAT(resized.visual_bounds().size.height, Gt(initial.visual_bounds().size.height));
}

TEST(MagnifierLayout, pinned_resize_holds_the_corner_exactly_across_magnifications)
{
    for (auto const magnification : {1.0f, 1.25f, 1.5f, 2.0f, 3.5f, 8.0f})
    {
        for (auto const dimension : {200, 213, 271, 340})
        {
            miral::MagnifierLayout const layout{{}, {dimension, dimension}, magnification};
            auto const anchor = layout.place_freely_at({100, 100}).resize_anchor();
            auto const dragged = anchor - geom::Displacement{dimension + 40, dimension + 40};

            EXPECT_THAT(layout.resize_from_pinned_corner(anchor, dragged).resize_anchor(), Eq(anchor))
                << "magnification " << magnification << ", size " << dimension;
        }
    }
}
TEST(MagnifierLayout, resize_past_the_output_cap_holds_size_and_position)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.25f};
    auto const pinned_corner = geom::Point{700, 550};

    auto const first = layout.resize_from_pinned_corner(pinned_corner, {50, 50});
    auto const second = layout.resize_from_pinned_corner(pinned_corner, {40, 40});

    EXPECT_THAT(first.visual_bounds(), Eq(second.visual_bounds()));
    EXPECT_THAT(first.resize_anchor(), Eq(pinned_corner));
    EXPECT_THAT(second.resize_anchor(), Eq(pinned_corner));
}

// The minimum is what the control layout demands: the resize handle and the
// zoom stack side by side across the top, and the zoom stack clear of the drag
// handle down the right.
TEST(MagnifierLayout, clamps_the_visual_size_to_fit_the_controls)
{
    miral::MagnifierLayout const layout{outputs(), {1, 1}, 1.0f};

    auto const bounds = layout.place_freely_at({100, 100}).visual_bounds();

    EXPECT_THAT(bounds.size, Eq(geom::Size{2 * 48, 2 * 48 + 8 + 48}));
}
