/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/display_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
using namespace testing;

namespace
{

mg::DisplayConfigurationOutput const tmpl_output
{
    mg::DisplayConfigurationOutputId{3},
    mg::DisplayConfigurationCardId{2},
    mg::DisplayConfigurationOutputType::dvid,
    {
        mir_pixel_format_abgr_8888
    },
    {
        {geom::Size{10, 20}, 60.0},
        {geom::Size{10, 20}, 59.0},
        {geom::Size{15, 20}, 59.0}
    },
    0,
    geom::Size{10, 20},
    true,
    true,
    geom::Point(),
    2,
    mir_pixel_format_abgr_8888,
    mir_power_mode_on,
    mir_orientation_normal,
    1.0f,
    mir_form_factor_monitor,
    mir_subpixel_arrangement_unknown,
    {},
    mir_output_gamma_unsupported,
    {},
    {}
};

}

TEST(DisplayConfiguration, same_cards_compare_equal)
{
    mg::DisplayConfigurationCardId const id{1};
    size_t const max_outputs{3};

    mg::DisplayConfigurationCard const card1{id, max_outputs};
    mg::DisplayConfigurationCard const card2 = card1;

    EXPECT_THAT(card1, Eq(card1));
    EXPECT_THAT(card1, Eq(card2));
    EXPECT_THAT(card2, Eq(card1));
}

TEST(DisplayConfiguration, different_cards_compare_unequal)
{
    mg::DisplayConfigurationCardId const id1{1};
    mg::DisplayConfigurationCardId const id2{2};
    size_t const max_outputs1{3};
    size_t const max_outputs2{4};

    mg::DisplayConfigurationCard const card1{id1, max_outputs1};
    mg::DisplayConfigurationCard const card2{id1, max_outputs2};
    mg::DisplayConfigurationCard const card3{id2, max_outputs1};

    EXPECT_THAT(card1, Ne(card2));
    EXPECT_THAT(card2, Ne(card1));
    EXPECT_THAT(card2, Ne(card3));
    EXPECT_THAT(card3, Ne(card2));
    EXPECT_THAT(card1, Ne(card3));
    EXPECT_THAT(card3, Ne(card1));
}

TEST(DisplayConfiguration, same_modes_compare_equal)
{
    geom::Size const size{10, 20};
    double const vrefresh{59.9};

    mg::DisplayConfigurationMode const mode1{size, vrefresh};
    mg::DisplayConfigurationMode const mode2 = mode1;

    EXPECT_THAT(mode1, Eq(mode1));
    EXPECT_THAT(mode1, Eq(mode2));
    EXPECT_THAT(mode2, Eq(mode1));
}

TEST(DisplayConfiguration, different_modes_compare_unequal)
{
    geom::Size const size1{10, 20};
    geom::Size const size2{10, 21};
    double const vrefresh1{59.9};
    double const vrefresh2{60.0};

    mg::DisplayConfigurationMode const mode1{size1, vrefresh1};
    mg::DisplayConfigurationMode const mode2{size1, vrefresh2};
    mg::DisplayConfigurationMode const mode3{size2, vrefresh1};

    EXPECT_THAT(mode1, Ne(mode2));
    EXPECT_THAT(mode2, Ne(mode1));
    EXPECT_THAT(mode2, Ne(mode3));
    EXPECT_THAT(mode3, Ne(mode2));
    EXPECT_THAT(mode1, Ne(mode3));
    EXPECT_THAT(mode3, Ne(mode1));
}

TEST(DisplayConfiguration, same_outputs_compare_equal)
{
    mg::DisplayConfigurationOutput const output1 = tmpl_output;
    mg::DisplayConfigurationOutput output2 = tmpl_output;

    EXPECT_THAT(output1, Eq(output1));
    EXPECT_THAT(output1, Eq(output2));
    EXPECT_THAT(output2, Eq(output1));
}

TEST(DisplayConfiguration, outputs_with_different_ids_compare_unequal)
{
    mg::DisplayConfigurationOutput const output1 = tmpl_output;
    mg::DisplayConfigurationOutput output2 = tmpl_output;
    mg::DisplayConfigurationOutput output3 = tmpl_output;

    output2.id = mg::DisplayConfigurationOutputId{15};
    output3.card_id = mg::DisplayConfigurationCardId{12};

    EXPECT_THAT(output1, Ne(output2));
    EXPECT_THAT(output2, Ne(output1));
    EXPECT_THAT(output2, Ne(output3));
    EXPECT_THAT(output3, Ne(output2));
    EXPECT_THAT(output1, Ne(output3));
    EXPECT_THAT(output3, Ne(output1));
}

TEST(DisplayConfiguration, outputs_with_different_modes_compare_unequal)
{
    mg::DisplayConfigurationOutput const output1 = tmpl_output;
    mg::DisplayConfigurationOutput output2 = tmpl_output;
    mg::DisplayConfigurationOutput output3 = tmpl_output;

    std::vector<mg::DisplayConfigurationMode> const modes2
    {
        {geom::Size{10, 20}, 60.0},
        {geom::Size{10, 20}, 59.9},
        {geom::Size{15, 20}, 59.0}
    };
    std::vector<mg::DisplayConfigurationMode> const modes3
    {
        {geom::Size{10, 20}, 60.0},
        {geom::Size{10, 20}, 59.0}
    };

    output2.modes = modes2;
    output3.modes = modes3;

    EXPECT_THAT(output1, Ne(output2));
    EXPECT_THAT(output2, Ne(output1));
    EXPECT_THAT(output2, Ne(output3));
    EXPECT_THAT(output3, Ne(output2));
    EXPECT_THAT(output1, Ne(output3));
    EXPECT_THAT(output3, Ne(output1));
}

TEST(DisplayConfiguration, outputs_with_different_physical_size_compare_unequal)
{
    mg::DisplayConfigurationOutput const output1 = tmpl_output;
    mg::DisplayConfigurationOutput output2 = tmpl_output;

    geom::Size const physical_size2{11, 20};

    output2.physical_size_mm = physical_size2;

    EXPECT_THAT(output1, Ne(output2));
    EXPECT_THAT(output2, Ne(output1));
}

TEST(DisplayConfiguration, outputs_with_different_connected_status_compare_unequal)
{
    mg::DisplayConfigurationOutput const output1 = tmpl_output;
    mg::DisplayConfigurationOutput output2 = tmpl_output;

    output2.connected = false;

    EXPECT_THAT(output1, Ne(output2));
    EXPECT_THAT(output2, Ne(output1));
}

TEST(DisplayConfiguration, outputs_with_different_current_mode_index_compare_unequal)
{
    mg::DisplayConfigurationOutput const output1 = tmpl_output;
    mg::DisplayConfigurationOutput output2 = tmpl_output;

    output2.current_mode_index = 0;

    EXPECT_THAT(output1, Ne(output2));
    EXPECT_THAT(output2, Ne(output1));
}

TEST(DisplayConfiguration, outputs_with_different_preferred_mode_index_compare_unequal)
{
    mg::DisplayConfigurationOutput const output1 = tmpl_output;
    mg::DisplayConfigurationOutput output2 = tmpl_output;

    output2.preferred_mode_index = 1;

    EXPECT_THAT(output1, Ne(output2));
    EXPECT_THAT(output2, Ne(output1));
}

TEST(DisplayConfiguration, outputs_with_different_orientation_compare_unequal)
{
    mg::DisplayConfigurationOutput a = tmpl_output;
    mg::DisplayConfigurationOutput b = tmpl_output;

    EXPECT_THAT(a, Eq(b));
    EXPECT_THAT(b, Eq(a));
    a.orientation = mir_orientation_left;
    b.orientation = mir_orientation_inverted;
    EXPECT_THAT(a, Ne(b));
    EXPECT_THAT(b, Ne(a));
}

TEST(DisplayConfiguration, outputs_with_different_power_mode_compare_equal)
{
    mg::DisplayConfigurationOutput a = tmpl_output;
    mg::DisplayConfigurationOutput b = tmpl_output;

    EXPECT_THAT(a, Eq(b));
    EXPECT_THAT(b, Eq(a));
    a.power_mode = mir_power_mode_on;
    b.power_mode = mir_power_mode_off;
    EXPECT_THAT(a, Eq(b));
    EXPECT_THAT(b, Eq(a));
}

TEST(DisplayConfiguration, outputs_with_different_scaling_factors_compare_unequal)
{
    mg::DisplayConfigurationOutput a = tmpl_output;
    mg::DisplayConfigurationOutput b = tmpl_output;

    EXPECT_THAT(a, Eq(b));
    EXPECT_THAT(b, Eq(a));
    a.scale = 2.0f;
    b.scale = 3.0f;
    EXPECT_THAT(a, Ne(b));
    EXPECT_THAT(b, Ne(a));
}

TEST(DisplayConfiguration, outputs_with_different_form_factors_compare_unequal)
{
    mg::DisplayConfigurationOutput a = tmpl_output;
    mg::DisplayConfigurationOutput b = tmpl_output;

    EXPECT_THAT(a, Eq(b));
    EXPECT_THAT(b, Eq(a));
    a.form_factor = mir_form_factor_monitor;
    b.form_factor = mir_form_factor_projector;
    EXPECT_THAT(a, Ne(b));
    EXPECT_THAT(b, Ne(a));
}

TEST(DisplayConfiguration, output_extents_uses_current_mode)
{
    mg::DisplayConfigurationOutput out = tmpl_output;

    out.current_mode_index = 2;
    ASSERT_THAT(out.modes[0], Ne(out.modes[2]));

    EXPECT_THAT(out.modes[out.current_mode_index].size, Eq(out.extents().size));
}

TEST(DisplayConfiguration, output_extents_are_customizable)
{
    mg::DisplayConfigurationOutput out = tmpl_output;

    geom::Size const custom_size{1234, 9876};
    out.custom_logical_size = custom_size;

    EXPECT_THAT(custom_size, Eq(out.extents().size));
}

TEST(DisplayConfiguration, output_extents_rotates_with_orientation)
{
    mg::DisplayConfigurationOutput out = tmpl_output;

    auto const& size = out.modes[out.current_mode_index].size;
    int w = size.width.as_int();
    int h = size.height.as_int();

    ASSERT_THAT(w, Ne(h));

    geom::Rectangle normal{out.top_left, {w, h}};
    geom::Rectangle swapped{out.top_left, {h, w}};

    out.orientation = mir_orientation_normal;
    EXPECT_THAT(normal, Eq(out.extents()));

    out.orientation = mir_orientation_inverted;
    EXPECT_THAT(normal, Eq(out.extents()));

    out.orientation = mir_orientation_left;
    EXPECT_THAT(swapped, Eq(out.extents()));

    out.orientation = mir_orientation_right;
    EXPECT_THAT(swapped, Eq(out.extents()));
}

TEST(DisplayConfiguration, default_valid)
{
    mg::DisplayConfigurationOutput out = tmpl_output;

    EXPECT_TRUE(out.valid());
}

TEST(DisplayConfiguration, used_and_disconnected_invalid)
{
    mg::DisplayConfigurationOutput out = tmpl_output;

    out.used = true;
    out.connected = false;

    EXPECT_FALSE(out.valid());
}

TEST(DisplayConfiguration, only_nonzero_custom_sizes_are_valid)
{
    mg::DisplayConfigurationOutput out = tmpl_output;

    out.custom_logical_size = {12, 34};
    EXPECT_TRUE(out.valid());

    out.custom_logical_size = {0, 56};
    EXPECT_FALSE(out.valid());

    out.custom_logical_size = {78, 0};
    EXPECT_FALSE(out.valid());

    out.custom_logical_size = {0, 0};
    EXPECT_FALSE(out.valid());
}

TEST(DisplayConfiguration, unsupported_format_invalid)
{
    mg::DisplayConfigurationOutput out = tmpl_output;
    out.current_format = mir_pixel_format_xbgr_8888;

    EXPECT_FALSE(out.valid());
}

TEST(DisplayConfiguration, unsupported_current_mode_invalid)
{
    mg::DisplayConfigurationOutput out = tmpl_output;
    out.current_mode_index = 123;

    EXPECT_FALSE(out.valid());
}

TEST(DisplayConfiguration, unsupported_preferred_mode_valid)
{   // Not having a preferred mode is allowed (LP: #1395405)
    mg::DisplayConfigurationOutput out = tmpl_output;
    out.preferred_mode_index = 456;

    EXPECT_TRUE(out.valid());
}

TEST(DisplayConfiguration, output_extents_empty_when_there_are_no_modes)
{
    mg::DisplayConfigurationOutput out = tmpl_output;

    out.modes.clear();
    out.current_mode_index = 0;

    geom::Rectangle empty{};
    EXPECT_THAT(empty, Eq(out.extents()));
}

TEST(DisplayConfiguration, output_extents_are_scaled)
{
    mg::DisplayConfigurationOutput out = tmpl_output;
    out.scale = 2.0f;

    EXPECT_THAT(out.extents().size, Eq(geom::Size{
        roundf(out.modes[out.current_mode_index].size.width.as_int() * 0.5),
        roundf(out.modes[out.current_mode_index].size.height.as_int() * 0.5)}));
}

TEST(DisplayConfiguration, output_extents_are_scaled_fractionally)
{
    mg::DisplayConfigurationOutput out = tmpl_output;
    out.scale = 0.8f;

    EXPECT_THAT(out.extents().size, Eq(geom::Size{
        roundf(out.modes[out.current_mode_index].size.width.as_int() * 1.25),
        roundf(out.modes[out.current_mode_index].size.height.as_int() * 1.25)}));
}

TEST(DisplayConfiguration, user_display_configuration_output_extents_are_scaled)
{
    mg::DisplayConfigurationOutput out = tmpl_output;
    mg::UserDisplayConfigurationOutput user{out};
    user.scale = 2.0f;

    EXPECT_THAT(user.extents().size, Eq(geom::Size{
        roundf(out.modes[out.current_mode_index].size.width.as_int() * 0.5),
        roundf(out.modes[out.current_mode_index].size.height.as_int() * 0.5)}));
}

TEST(DisplayConfiguration, user_display_configuration_output_extents_are_scaled_fractionally)
{
    mg::DisplayConfigurationOutput out = tmpl_output;
    mg::UserDisplayConfigurationOutput user{out};
    user.scale = 0.8f;

    EXPECT_THAT(user.extents().size, Eq(geom::Size{
        roundf(out.modes[out.current_mode_index].size.width.as_int() * 1.25),
        roundf(out.modes[out.current_mode_index].size.height.as_int() * 1.25)}));
}
