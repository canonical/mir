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
#include "magnifier_layout.h"

#include <mir/flags.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <tuple>

namespace geom = mir::geometry;
namespace controls = miral::magnifier_controls;
namespace layout = miral::magnifier_layout;
using namespace testing;

namespace
{
enum class ExpectedEdge
{
    left = 1 << 0,
    right = 1 << 1,
    top = 1 << 2,
    bottom = 1 << 3,
};

constexpr auto mir_enable_enum_bit_operators(ExpectedEdge edge) -> ExpectedEdge
{
    return edge;
}

using ExpectedEdges = mir::Flags<ExpectedEdge>;

constexpr std::array test_magnifications{1.25, 1.5, 2.0, 3.5, 8.0};

auto single_output(geom::Rectangle const& output = {{0, 0}, {800, 600}}) -> geom::Rectangles
{
    return {output};
}

auto surface_top_left(layout::Placement const& placement) -> geom::Point
{
    return placement.capture_area.top_left;
}

auto surface_top_left(layout::FreePlacement const& placement) -> geom::Point
{
    return placement.surface_top_left;
}

auto visual_bounds(auto const& placement, double const magnification) -> geom::RectangleD
{
    return miral::magnifier_geometry::visual_bounds(
        surface_top_left(placement),
        placement.capture_area.size,
        magnification);
}

auto surface_center(auto const& placement) -> geom::PointD
{
    return {
        surface_top_left(placement).x.as_value() +
            placement.capture_area.size.width.as_value() / 2.0,
        surface_top_left(placement).y.as_value() +
            placement.capture_area.size.height.as_value() / 2.0};
}

auto capture_center(layout::Placement const& placement) -> geom::PointD
{
    return {
        placement.capture_area.left().as_value() + placement.capture_area.size.width.as_value() / 2.0,
        placement.capture_area.top().as_value() + placement.capture_area.size.height.as_value() / 2.0};
}

void expect_center_near(geom::PointD const& actual, geom::PointD const& expected, double const tolerance = 1.0)
{
    EXPECT_THAT(actual.x.as_value(), DoubleNear(expected.x.as_value(), tolerance));
    EXPECT_THAT(actual.y.as_value(), DoubleNear(expected.y.as_value(), tolerance));
}

class FollowingCursorPlacement : public TestWithParam<std::tuple<double, geom::SizeD>>
{
};

class IntegralTranslation : public TestWithParam<double>
{
};

class PinnedResize : public TestWithParam<double>
{
};

class ContentHitTestingForHandle : public TestWithParam<controls::HandleKind>
{
};

struct OutputSelectionParameters
{
    geom::PointD desired_center;
    geom::Size expected_capture_size;
};

class OutputSelection : public TestWithParam<std::tuple<geom::Rectangles, OutputSelectionParameters>>
{
};

struct EdgeAndCornerPlacementParameters
{
    geom::PointD center;
    ExpectedEdges expected_edges;
};

class EdgeAndCornerPlacement : public TestWithParam<EdgeAndCornerPlacementParameters>
{
};
}

TEST_P(FollowingCursorPlacement, couples_capture_and_surface_with_bounded_center_error)
{
    auto const outputs = single_output({{-1000, -700}, {1800, 1300}});
    auto const& [magnification, size] = GetParam();

    auto const cursor = geom::PointD{-123.5, 47.25};
    auto const placement =
        layout::place_following_cursor(cursor, size, outputs, magnification);

    EXPECT_THAT(
        placement.capture_area,
        Eq(geom::Rectangle{
            surface_top_left(placement),
            placement.capture_area.size}));
    expect_center_near(surface_center(placement), cursor);
}

INSTANTIATE_TEST_SUITE_P(
    MagnificationsAndSizes,
    FollowingCursorPlacement,
    Combine(
        ValuesIn(test_magnifications),
        Values(geom::SizeD{201.0, 213.0}, geom::SizeD{240.0, 200.0})));

TEST(MagnifierLayout, following_cursor_is_not_confined_at_output_edges)
{
    auto const outputs = single_output();

    auto const placement =
        layout::place_following_cursor({0.0, 0.0}, {200.0, 200.0}, outputs, 2.0);

    auto const bounds = visual_bounds(placement, 2.0);
    EXPECT_THAT(bounds.left().as_value(), DoubleEq(-100.0));
    EXPECT_THAT(bounds.top().as_value(), DoubleEq(-100.0));
}

TEST(MagnifierLayout, free_placement_at_output_center_aligns_capture_and_surface_centers)
{
    auto const outputs = single_output();

    auto const placement =
        layout::place_freely({400.0, 300.0}, {240.0, 200.0}, outputs, 2.0);

    expect_center_near(surface_center(placement), {400.0, 300.0}, 0.0);
    expect_center_near(capture_center(placement), surface_center(placement), 0.0);
}

TEST_P(EdgeAndCornerPlacement, preserves_handle_width_of_source_clearance)
{
    auto const output = geom::Rectangle{{0, 0}, {800, 600}};
    auto const outputs = single_output(output);
    auto const requested_size = geom::SizeD{240.0, 200.0};
    auto const magnification = 2.0;
    auto const [center, expected_edges] = GetParam();

    auto const placement = layout::place_freely(center, requested_size, outputs, magnification);
    auto const bounds = visual_bounds(placement, magnification);

    auto const left_visual_clearance =
        (output.left() - placement.capture_area.left()).as_value() * magnification;
    auto const right_visual_clearance =
        (placement.capture_area.right() - output.right()).as_value() * magnification;
    auto const top_visual_clearance =
        (output.top() - placement.capture_area.top()).as_value() * magnification;
    auto const bottom_visual_clearance =
        (placement.capture_area.bottom() - output.bottom()).as_value() * magnification;

    expect_center_near(surface_center(placement), center, 0.0);

    if (contains(expected_edges, ExpectedEdge::left))
    {
        EXPECT_THAT(bounds.left().as_value(), DoubleEq(output.left().as_value()));
        EXPECT_GE(left_visual_clearance, controls::handle_diameter);
    }

    if (contains(expected_edges, ExpectedEdge::right))
    {
        EXPECT_THAT(bounds.right().as_value(), DoubleEq(output.right().as_value()));
        EXPECT_GE(right_visual_clearance, controls::handle_diameter);
    }

    if (contains(expected_edges, ExpectedEdge::top))
    {
        EXPECT_THAT(bounds.top().as_value(), DoubleEq(output.top().as_value()));
        EXPECT_GE(top_visual_clearance, controls::handle_diameter);
    }

    if (contains(expected_edges, ExpectedEdge::bottom))
    {
        EXPECT_THAT(bounds.bottom().as_value(), DoubleEq(output.bottom().as_value()));
        EXPECT_GE(bottom_visual_clearance, controls::handle_diameter);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OutputEdgesAndCorners,
    EdgeAndCornerPlacement,
    Values(
        EdgeAndCornerPlacementParameters{.center = {120.0, 300.0}, .expected_edges = ExpectedEdge::left},
        EdgeAndCornerPlacementParameters{.center = {680.0, 300.0}, .expected_edges = ExpectedEdge::right},
        EdgeAndCornerPlacementParameters{.center = {400.0, 100.0}, .expected_edges = ExpectedEdge::top},
        EdgeAndCornerPlacementParameters{.center = {400.0, 500.0}, .expected_edges = ExpectedEdge::bottom},
        EdgeAndCornerPlacementParameters{
            .center = {120.0, 100.0},
            .expected_edges = ExpectedEdge::left | ExpectedEdge::top},
        EdgeAndCornerPlacementParameters{
            .center = {680.0, 500.0},
            .expected_edges = ExpectedEdge::right | ExpectedEdge::bottom}));

TEST(MagnifierLayout, source_clearance_is_capped_to_less_than_half_the_capture)
{
    auto const outputs = single_output();
    auto const magnification = 3.5;
    auto const placement = layout::place_freely(
        {47.25, 300.0},
        geom::SizeD{96.0, 152.0},
        outputs,
        magnification);
    auto const bounds = visual_bounds(placement, magnification);

    EXPECT_THAT(placement.capture_area.size.width, Eq(geom::Width{27}));
    EXPECT_THAT(std::ceil(controls::visual_edge_clearance / magnification), Gt(27 / 2.0));
    EXPECT_LT(bounds.left().as_value(), 0.5);
    EXPECT_THAT(placement.capture_area.left(), Eq(geom::X{-13}));
}

TEST_P(OutputSelection, capture_size_matches_selected_output)
{
    auto const& [outputs, parameters] = GetParam();
    auto const placement =
        layout::place_following_cursor(parameters.desired_center, {1000.0, 1000.0}, outputs, 1.0);

    EXPECT_THAT(placement.capture_area.size, Eq(parameters.expected_capture_size));
}

INSTANTIATE_TEST_SUITE_P(
    DesiredCenterSelectsContainingOutput,
    OutputSelection,
    Combine(
        Values(geom::Rectangles{{{0, 0}, {800, 600}}, {{800, 0}, {400, 300}}}),
        Values(
            OutputSelectionParameters{
                .desired_center = {799.0, 150.0},
                .expected_capture_size = {640, 480}},
            OutputSelectionParameters{
                .desired_center = {801.0, 150.0},
                .expected_capture_size = {320, 240}})));

INSTANTIATE_TEST_SUITE_P(
    DesiredCenterInGapSelectsNearestOutput,
    OutputSelection,
    Combine(
        Values(geom::Rectangles{{{0, 0}, {800, 600}}, {{1000, 0}, {400, 300}}}),
        Values(
            OutputSelectionParameters{
                .desired_center = {850.0, 150.0},
                .expected_capture_size = {640, 480}},
            OutputSelectionParameters{
                .desired_center = {950.0, 150.0},
                .expected_capture_size = {320, 240}})));

TEST(MagnifierLayout, adjacent_output_straddling_is_not_confined)
{
    geom::Rectangles outputs{
        {{0, 0}, {800, 600}},
        {{800, 0}, {400, 600}}};

    auto const placement = layout::place_freely({800.0, 300.0}, {200.0, 200.0}, outputs, 1.0);

    EXPECT_THAT(visual_bounds(placement, 1.0).left().as_value(), DoubleEq(700.0));
    expect_center_near(surface_center(placement), {800.0, 300.0}, 0.0);
}

TEST(MagnifierLayout, exactly_flush_far_edge_is_covered_without_confinement)
{
    auto const outputs = single_output();

    auto const placement = layout::place_freely({700.0, 300.0}, {200.0, 200.0}, outputs, 1.25);

    EXPECT_THAT(visual_bounds(placement, 1.25).right().as_value(), DoubleEq(800.0));
    expect_center_near(surface_center(placement), {700.0, 300.0}, 0.0);
}

TEST(MagnifierLayout, gap_overhang_is_confined_with_rounding_toward_containment)
{
    auto const selected_output = geom::Rectangle{{0, 0}, {800, 600}};
    geom::Rectangles outputs{
        selected_output,
        {{900, 0}, {400, 600}}};
    auto const desired_center = geom::PointD{842.0, 300.0};
    auto const magnification = 1.25;

    auto const placement =
        layout::place_freely(desired_center, {96.0, 200.0}, outputs, magnification);
    auto const bounds = visual_bounds(placement, magnification);

    EXPECT_LE(bounds.right().as_value(), selected_output.right().as_value());
    EXPECT_GT(bounds.right().as_value(), selected_output.right().as_value() - 1.0);
    EXPECT_LT(surface_center(placement).x.as_value(), desired_center.x.as_value());
}

TEST(MagnifierLayout, left_and_top_confinement_rounds_right_and_down_toward_containment)
{
    auto const output = geom::Rectangle{{0, 0}, {800, 600}};
    auto const desired_center = geom::PointD{-42.0, -42.0};
    auto const magnification = 1.25;

    auto const placement =
        layout::place_freely(desired_center, {96.0, 152.0}, single_output(output), magnification);
    auto const bounds = visual_bounds(placement, magnification);

    EXPECT_GE(bounds.left().as_value(), output.left().as_value());
    EXPECT_GE(bounds.top().as_value(), output.top().as_value());
    EXPECT_LT(bounds.left().as_value(), output.left().as_value() + 1.0);
    EXPECT_LT(bounds.top().as_value(), output.top().as_value() + 1.0);
    EXPECT_GT(surface_center(placement).x.as_value(), desired_center.x.as_value());
    EXPECT_GT(surface_center(placement).y.as_value(), desired_center.y.as_value());
}

TEST(MagnifierLayout, output_removal_relocates_to_nearest_remaining_output)
{
    auto const outputs = single_output();

    auto const placement = layout::place_freely({2100.0, 300.0}, {200.0, 200.0}, outputs, 1.0);
    auto const bounds = visual_bounds(placement, 1.0);

    EXPECT_THAT(bounds.right().as_value(), DoubleEq(800.0));
    expect_center_near(surface_center(placement), {700.0, 300.0}, 0.0);
}

TEST(MagnifierLayout, capture_rounding_never_exceeds_strict_eighty_percent_cap)
{
    constexpr auto maximum_visual_fraction = 0.8;
    auto const output = geom::Rectangle{{0, 0}, {201, 199}};
    auto const magnification = 1.25;

    auto const placement =
        layout::place_following_cursor(
            {50.0, 49.0},
            {1000.0, 1000.0},
            single_output(output),
            magnification);
    auto const bounds = visual_bounds(placement, magnification);

    EXPECT_LE(
        bounds.size.width.as_value(),
        maximum_visual_fraction * output.size.width.as_value());
    EXPECT_LE(
        bounds.size.height.as_value(),
        maximum_visual_fraction * output.size.height.as_value());
    EXPECT_THAT(placement.capture_area.size, Eq(geom::Size{128, 127}));
}

TEST(MagnifierLayout, visual_size_is_clamped_to_the_control_minimum)
{
    auto const outputs = single_output();

    auto const placement =
        layout::place_freely({400.0, 300.0}, {1.0, 1.0}, outputs, 1.0);

    EXPECT_THAT(placement.capture_area.size, Eq(controls::minimum_visual_size));
}

TEST(MagnifierLayout, requested_size_is_reclamped_for_each_output)
{
    auto const small_outputs = single_output({{0, 0}, {400, 300}});
    auto const large_outputs = single_output({{0, 0}, {1600, 1200}});
    auto const requested = geom::SizeD{1000.0, 900.0};
    auto const magnification = 2.0;

    auto const small =
        layout::place_freely({200.0, 150.0}, requested, small_outputs, magnification);
    auto const large =
        layout::place_freely({800.0, 600.0}, requested, large_outputs, magnification);

    // The request is output-limited on the small output and request-limited on the large output.
    EXPECT_THAT(small.capture_area.size, Eq(geom::Size{160, 120}));
    EXPECT_THAT(large.capture_area.size, Eq(geom::Size{500, 450}));
}

TEST_P(IntegralTranslation, has_bounded_quantization_error)
{
    auto const magnification = GetParam();
    auto const translation = geom::Displacement{37, -41};
    auto const requested_size = geom::SizeD{213.0, 271.0};

    auto const original_output = geom::Rectangle{{-1000, -700}, {1800, 1300}};
    auto const translated_output = geom::Rectangle{original_output.top_left + translation, original_output.size};

    auto const original_center = geom::PointD{-123.5, 47.25};
    auto const translated_center = original_center + geom::DisplacementD{translation};

    auto const original = layout::place_freely(
        original_center,
        requested_size,
        single_output(original_output),
        magnification);

    auto const translated = layout::place_freely(
        translated_center,
        requested_size,
        single_output(translated_output),
        magnification);

    expect_center_near(surface_center(original), original_center);
    expect_center_near(surface_center(translated), translated_center);

    auto const surface_translation = translated.surface_top_left - original.surface_top_left;
    auto const capture_translation = translated.capture_area.top_left - original.capture_area.top_left;

    auto const expected_dx = translation.dx.as_value();
    auto const expected_dy = translation.dy.as_value();

    EXPECT_THAT(surface_translation.dx.as_value(), Eq(expected_dx));
    EXPECT_THAT(capture_translation.dx.as_value(), Eq(expected_dx));

    // Each integer position truncates toward zero, so the measured translation is
    // trunc(y + dy) - trunc(y). At 3.5x, the surface positions are 8.75 and -32.25:
    // trunc(-32.25) - trunc(8.75) = -32 - 8 = -40, one pixel less negative than -41.
    // Mapping from those quantized surface positions gives capture positions 28.648... and
    // -11.137...: trunc(-11.137...) - trunc(28.648...) = -11 - 28 = -39, two pixels less negative.
    EXPECT_THAT(surface_translation.dy.as_value(), AllOf(Ge(expected_dy), Le(expected_dy + 1)));
    EXPECT_THAT(capture_translation.dy.as_value(), AllOf(Ge(expected_dy), Le(expected_dy + 2)));

    EXPECT_THAT(translated.capture_area.size, Eq(original.capture_area.size));
}

INSTANTIATE_TEST_SUITE_P(
    Magnifications,
    IntegralTranslation,
    ValuesIn(test_magnifications));

TEST_P(PinnedResize, keeps_bottom_right_handle_position)
{
    auto const outputs = single_output();
    auto const pinned_handle_corner = geom::Point{700, 550};
    auto const pinned = geom::PointD{pinned_handle_corner};
    auto const magnification = GetParam();

    auto const placement = layout::resize_freely(
        {460.0, 310.0},
        pinned,
        outputs,
        magnification);
    auto const bounds = visual_bounds(placement, magnification);
    auto const handles = controls::positions_for(placement, magnification);
    auto const actual_handle_corner =
        handles.drag + geom::Displacement{controls::handle_diameter, controls::handle_diameter};

    EXPECT_THAT(actual_handle_corner, Eq(pinned_handle_corner));
    EXPECT_THAT(bounds.right().as_value(), DoubleNear(pinned.x.as_value(), 0.5));
    EXPECT_THAT(bounds.bottom().as_value(), DoubleNear(pinned.y.as_value(), 0.5));
}

INSTANTIATE_TEST_SUITE_P(
    Magnifications,
    PinnedResize,
    ValuesIn(test_magnifications));

TEST(MagnifierLayout, pinned_resize_clamps_to_output_cap_without_hidden_oversize)
{
    constexpr auto maximum_visual_fraction = 0.8;
    auto const output = geom::Rectangle{{0, 0}, {800, 600}};
    auto const outputs = single_output(output);
    auto const pinned = geom::PointD{750.0, 550.0};
    auto const magnification = 1.25;

    auto const first = layout::resize_freely({0.0, 0.0}, pinned, outputs, magnification);
    auto const farther = layout::resize_freely({-100.0, -100.0}, pinned, outputs, magnification);
    auto const first_bounds = visual_bounds(first, magnification);

    EXPECT_THAT(first.capture_area.size, Eq(geom::Size{512, 384}));
    EXPECT_THAT(farther.capture_area.size, Eq(first.capture_area.size));
    EXPECT_LE(
        first_bounds.size.width.as_value(),
        maximum_visual_fraction * output.size.width.as_value());
    EXPECT_LE(
        first_bounds.size.height.as_value(),
        maximum_visual_fraction * output.size.height.as_value());
}

TEST(MagnifierControls, positions_are_derived_from_visual_bounds)
{
    auto const magnification = 1.25;
    layout::FreePlacement const placement{
        layout::Placement{{{0, 0}, {161, 121}}},
        {100, 80}};
    auto const bounds = visual_bounds(placement, magnification);

    EXPECT_THAT(bounds.size.width.as_value(), DoubleEq(201.25));

    auto const positions = controls::positions_for(placement, magnification);
    EXPECT_THAT(positions.resize, Eq(geom::Point{79, 64}));
    EXPECT_THAT(positions.drag, Eq(geom::Point{233, 168}));
    EXPECT_THAT(positions.zoom_in, Eq(geom::Point{233, 64}));
    EXPECT_THAT(positions.zoom_out, Eq(geom::Point{233, 120}));
}

TEST(MagnifierControls, content_hit_testing_includes_content_and_excludes_outside_bounds)
{
    auto const outputs = single_output();
    auto const magnification = 1.0;
    auto const placement =
        layout::place_freely({400.0, 300.0}, {240.0, 200.0}, outputs, magnification);

    EXPECT_TRUE(controls::contains_content(placement, magnification, {400.0f, 300.0f}));
    EXPECT_FALSE(controls::contains_content(placement, magnification, {279.0f, 300.0f}));
    EXPECT_FALSE(controls::contains_content(placement, magnification, {521.0f, 300.0f}));
}

TEST_P(ContentHitTestingForHandle, excludes_handles)
{
    auto const magnification = 1.0;
    auto const placement =
        layout::place_freely({400.0, 300.0}, {240.0, 200.0}, single_output(), magnification);
    auto const positions = controls::positions_for(placement, magnification);
    auto const handle = positions.for_kind(GetParam());
    auto const handle_center = geom::PointD{
        handle.x.as_value() + controls::handle_diameter / 2.0,
        handle.y.as_value() + controls::handle_diameter / 2.0};

    EXPECT_FALSE(controls::contains_content(placement, magnification, handle_center));
}

INSTANTIATE_TEST_SUITE_P(
    EveryControl,
    ContentHitTestingForHandle,
    Values(
        controls::HandleKind::drag,
        controls::HandleKind::resize,
        controls::HandleKind::zoom_in,
        controls::HandleKind::zoom_out));
