/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/graphics/nested/nested_display_configuration.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;
using namespace testing;

namespace
{
uint32_t const default_num_modes = 0;
MirDisplayMode* const default_modes = nullptr;
uint32_t const default_preferred_mode = 0;
uint32_t const default_current_mode = 0;
uint32_t const default_num_output_formats = 0;
MirPixelFormat* const default_output_formats = nullptr;
uint32_t const default_current_output_format = 0;
uint32_t const default_card_id = 1;
uint32_t const second_card_id = 2;
uint32_t const default_output_id = 0;
uint32_t const second_output_id = 1;
uint32_t const third_output_id = 2;
auto const default_type = MirDisplayOutputType(0);
int32_t const default_position_x = 0;
int32_t const default_position_y = 0;
uint32_t const default_connected = 0;
uint32_t const default_used = 0;
uint32_t const default_physical_width_mm = 0;
uint32_t const default_physical_height_mm = 0;

struct NestedDisplayConfiguration : public ::testing::Test
{
    template<int NoOfOutputs, int NoOfCards>
    MirDisplayConfiguration* build_test_config(
        MirDisplayOutput const (&outputs)[NoOfOutputs],
        MirDisplayCard const (&cards)[NoOfCards])
    {
        auto out_tmp = new MirDisplayOutput[NoOfOutputs];
        std::copy(outputs, outputs+NoOfOutputs, out_tmp);

        auto card_tmp = new MirDisplayCard[NoOfCards];
        std::copy(cards, cards+NoOfCards, card_tmp);

        return new MirDisplayConfiguration{ NoOfOutputs, out_tmp, NoOfCards, card_tmp };
    }

    template<int NoOfModes, int NoOfFormats>
    void init_output(
        MirDisplayOutput* output,
        MirDisplayMode const (&modes)[NoOfModes],
        MirPixelFormat const (&formats)[NoOfFormats])
    {
        auto mode_tmp = new MirDisplayMode[NoOfModes];
        std::copy(modes, modes+NoOfModes, mode_tmp);

        auto format_tmp = new MirPixelFormat[NoOfFormats];
        std::copy(formats, formats+NoOfFormats, format_tmp);

        output->num_modes = NoOfModes;
        output->modes = mode_tmp;
        output->preferred_mode = 0;
        output->current_mode = 0;
        output->num_output_formats = NoOfFormats;
        output->output_formats = format_tmp;
        output->current_output_format = 0;
    }

    template<int NoOfOutputs, int NoOfModes, int NoOfFormats>
    void init_outputs(
        MirDisplayOutput (&outputs)[NoOfOutputs],
        MirDisplayMode const (&modes)[NoOfModes],
        MirPixelFormat const (&formats)[NoOfFormats])
    {
        for(auto output = outputs; output != outputs+NoOfOutputs; ++output)
            init_output(output, modes, formats);
    }

    MirDisplayConfiguration* build_trivial_configuration()
    {
        static MirDisplayCard const cards[] {{default_card_id,1}};
        static MirDisplayMode const modes[] = {{ 1080, 1920, 4.33f }};
        static MirPixelFormat const formats[] = { mir_pixel_format_abgr_8888 };

        MirDisplayOutput outputs[] {{
            default_num_modes,
            default_modes,
            default_preferred_mode,
            default_current_mode,
            default_num_output_formats,
            default_output_formats,
            default_current_output_format,
            default_card_id,
            default_output_id,
            default_type,
            default_position_x,
            default_position_y,
            default_connected,
            default_used,
            default_physical_width_mm,
            default_physical_height_mm
        }};

        init_output(outputs, modes, formats);

        return build_test_config(outputs, cards);
    }

    MirDisplayConfiguration* build_non_trivial_configuration()
    {
        static MirDisplayCard const cards[] {
            {default_card_id,1},
            {second_card_id,2}};

        static MirDisplayMode const modes[] = {
            { 1080, 1920, 4.33f },
            { 1080, 1920, 1.11f }};
        static MirPixelFormat const formats[] = {
            mir_pixel_format_abgr_8888,
            mir_pixel_format_xbgr_8888,
            mir_pixel_format_argb_8888,
            mir_pixel_format_xrgb_8888,
            mir_pixel_format_bgr_888};

        MirDisplayOutput outputs[] {
            {
                default_num_modes,
                default_modes,
                default_preferred_mode,
                default_current_mode,
                default_num_output_formats,
                default_output_formats,
                default_current_output_format,
                default_card_id,
                default_output_id,
                default_type,
                default_position_x,
                default_position_y,
                default_connected,
                default_used,
                default_physical_width_mm,
                default_physical_height_mm
            },
            {
                default_num_modes,
                default_modes,
                default_preferred_mode,
                default_current_mode,
                default_num_output_formats,
                default_output_formats,
                default_current_output_format,
                second_card_id,
                second_output_id,
                default_type,
                default_position_x,
                default_position_y,
                default_connected,
                default_used,
                default_physical_width_mm,
                default_physical_height_mm
            },
            {
                default_num_modes,
                default_modes,
                default_preferred_mode,
                default_current_mode,
                default_num_output_formats,
                default_output_formats,
                default_current_output_format,
                second_card_id,
                third_output_id,
                default_type,
                default_position_x,
                default_position_y,
                default_connected,
                default_used,
                default_physical_width_mm,
                default_physical_height_mm
            },
        };

        init_outputs(outputs, modes, formats);

        return build_test_config(outputs, cards);
    }
};

struct MockCardVisitor
{
    MOCK_CONST_METHOD1(f, void(mg::DisplayConfigurationCard const&));
};

struct MockOutputVisitor
{
    MOCK_CONST_METHOD1(f, void(mg::DisplayConfigurationOutput const&));
};

}

TEST_F(NestedDisplayConfiguration, empty_configuration_is_read_correctly)
{
    auto empty_configuration = new MirDisplayConfiguration{ 0, nullptr, 0, nullptr };
    mgn::NestedDisplayConfiguration config(empty_configuration);

    config.for_each_card([](mg::DisplayConfigurationCard const&) { FAIL(); });
    config.for_each_output([](mg::DisplayConfigurationOutput const&) { FAIL(); });
}

TEST_F(NestedDisplayConfiguration, trivial_configuration_has_one_card)
{
    mgn::NestedDisplayConfiguration config(build_trivial_configuration());

    MockCardVisitor cv;
    EXPECT_CALL(cv, f(_)).Times(Exactly(1));

    config.for_each_card([&cv](mg::DisplayConfigurationCard const& card) { cv.f(card); });
}

TEST_F(NestedDisplayConfiguration, trivial_configuration_has_one_output)
{
    mgn::NestedDisplayConfiguration config(build_trivial_configuration());

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(1));

    config.for_each_output([&ov](mg::DisplayConfigurationOutput const& output) { ov.f(output); });
}

TEST_F(NestedDisplayConfiguration, trivial_configuration_can_be_configured)
{
    geom::Point const top_left{10,20};
    mgn::NestedDisplayConfiguration config(build_trivial_configuration());

    config.configure_output(mg::DisplayConfigurationOutputId(default_output_id), true, top_left, default_current_mode);

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(1));

    config.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            ov.f(output);
            EXPECT_EQ(true, output.used);
            EXPECT_EQ(top_left, output.top_left);
            EXPECT_EQ(0, output.current_mode_index);
        });
}

TEST_F(NestedDisplayConfiguration, configure_output_rejects_invalid_mode)
{
    geom::Point const top_left{10,20};
    size_t const too_big_mode_index = 1;
    mgn::NestedDisplayConfiguration config(build_trivial_configuration());

    EXPECT_THROW(
        {config.configure_output(mg::DisplayConfigurationOutputId(default_output_id), true, top_left, -1);},
        std::runtime_error);

    EXPECT_THROW(
        {config.configure_output(mg::DisplayConfigurationOutputId(default_output_id), true, top_left, too_big_mode_index);},
        std::runtime_error);
}

TEST_F(NestedDisplayConfiguration, configure_output_rejects_invalid_output)
{
    geom::Point const top_left{10,20};
    mgn::NestedDisplayConfiguration config(build_trivial_configuration());

    EXPECT_THROW(
        {config.configure_output(mg::DisplayConfigurationOutputId(default_output_id+1), true, top_left, default_current_mode);},
        std::runtime_error);

    EXPECT_THROW(
        {config.configure_output(mg::DisplayConfigurationOutputId(default_output_id-1), true, top_left, default_current_mode);},
        std::runtime_error);
}

TEST_F(NestedDisplayConfiguration, non_trivial_configuration_has_two_cards)
{
    mgn::NestedDisplayConfiguration config(build_non_trivial_configuration());

    MockCardVisitor cv;
    EXPECT_CALL(cv, f(_)).Times(Exactly(2));

    config.for_each_card([&cv](mg::DisplayConfigurationCard const& card) { cv.f(card); });
}

TEST_F(NestedDisplayConfiguration, non_trivial_configuration_has_three_outputs)
{
    mgn::NestedDisplayConfiguration config(build_non_trivial_configuration());

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(3));

    config.for_each_output([&ov](mg::DisplayConfigurationOutput const& output) { ov.f(output); });
}

TEST_F(NestedDisplayConfiguration, non_trivial_configuration_can_be_configured)
{
    mg::DisplayConfigurationOutputId const id(second_output_id);
    geom::Point const top_left{100,200};
    mgn::NestedDisplayConfiguration config(build_non_trivial_configuration());

    config.configure_output(id, true, top_left, 1);

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(3));
    config.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            ov.f(output);
            if (output.id == id)
            {
                EXPECT_EQ(true, output.used);
                EXPECT_EQ(top_left, output.top_left);
                EXPECT_EQ(1, output.current_mode_index);
            }
        });
}
