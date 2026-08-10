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

    auto const placement = layout.centered_on({400, 300});

    EXPECT_THAT(placement.visual_size, Eq(geom::Size{640, 480}));
    EXPECT_THAT(placement.capture_area, Eq(geom::Rectangle{{240, 180}, {320, 240}}));
    EXPECT_THAT(placement.surface_top_left, Eq(placement.capture_area.top_left));
}

TEST(MagnifierLayout, freely_positioned_surface_is_clamped_to_output)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};

    auto const placement = layout.freely_positioned({-50, -50});

    EXPECT_THAT(placement.surface_top_left, Eq(geom::Point{0, 0}));
}

TEST(MagnifierLayout, chooses_output_using_visual_bounds)
{
    auto bounds = outputs();
    bounds.add({{800, 0}, {400, 300}});
    miral::MagnifierLayout const layout{bounds, {500, 500}, 2.0f};

    auto const placement = layout.centered_on({790, 150});

    EXPECT_THAT(placement.visual_size, Eq(geom::Size{500, 480}));
}

TEST(MagnifierLayout, computes_distinct_zoom_button_positions)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};
    auto const placement = layout.freely_positioned({100, 100});

    auto const positions = layout.handle_positions(placement);

    EXPECT_THAT(positions.zoom_out.x, Eq(positions.zoom_in.x));
    EXPECT_THAT(positions.zoom_out.y - positions.zoom_in.y, Eq(geom::DeltaY{56}));
}

TEST(MagnifierLayout, content_hit_testing_excludes_controls)
{
    miral::MagnifierLayout const layout{outputs(), {200, 200}, 1.0f};
    auto const placement = layout.freely_positioned({100, 100});
    auto const positions = layout.handle_positions(placement);

    EXPECT_TRUE(layout.contains_content({200.0f, 200.0f}, placement));
    EXPECT_FALSE(layout.contains_content(
        {
            static_cast<float>(positions.drag.x.as_value() + miral::MagnifierLayout::handle_diameter / 2),
            static_cast<float>(positions.drag.y.as_value() + miral::MagnifierLayout::handle_diameter / 2),
        },
        placement));
}

TEST(MagnifierLayout, pinned_resize_preserves_opposite_visual_corner)
{
    miral::MagnifierLayout const layout{{}, {200, 200}, 1.25f};
    auto const initial = layout.freely_positioned({100, 100});
    auto const anchor = layout.resize_anchor(initial);
    auto const resize_handle = layout.handle_positions(initial).resize;

    auto const resized = layout.resized_from_pinned_corner(
        anchor,
        {resize_handle.x - geom::DeltaX{20}, resize_handle.y - geom::DeltaY{20}});

    EXPECT_THAT(layout.resize_anchor(resized), Eq(anchor));
    EXPECT_THAT(resized.visual_size.width, Gt(initial.visual_size.width));
    EXPECT_THAT(resized.visual_size.height, Gt(initial.visual_size.height));
}
