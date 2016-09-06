/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_display_configuration_builder.h"

#include <algorithm>

namespace mt = mir::test;

namespace
{

uint32_t const default_num_modes = 0;
MirDisplayMode* const default_modes = nullptr;
uint32_t const default_preferred_mode = 0;
uint32_t const default_current_mode = 0;
uint32_t const default_num_output_formats = 0;
MirPixelFormat* const default_output_formats = nullptr;
MirPixelFormat const default_current_output_format = mir_pixel_format_abgr_8888;
uint32_t const default_card_id = 1;
uint32_t const second_card_id = 2;
uint32_t const default_output_id = 0;
uint32_t const second_output_id = 1;
uint32_t const third_output_id = 2;
auto const default_type = MirDisplayOutputType(0);
int32_t const default_position_x = 0;
int32_t const default_position_y = 0;
uint32_t const default_connected = 1;
uint32_t const default_used = 1;
uint32_t const default_physical_width_mm = 0;
uint32_t const default_physical_height_mm = 0;

template<int NoOfOutputs, int NoOfCards>
std::shared_ptr<MirDisplayConfiguration> build_test_config(
    MirDisplayOutput const (&outputs)[NoOfOutputs],
    MirDisplayCard const (&cards)[NoOfCards])
{
    auto out_tmp = new MirDisplayOutput[NoOfOutputs];
    std::copy(outputs, outputs+NoOfOutputs, out_tmp);

    auto card_tmp = new MirDisplayCard[NoOfCards];
    std::copy(cards, cards+NoOfCards, card_tmp);

    return std::shared_ptr<MirDisplayConfiguration>(
        new MirDisplayConfiguration{NoOfOutputs, out_tmp, NoOfCards, card_tmp},
        [] (MirDisplayConfiguration* conf)
        {
            std::for_each(
                conf->outputs, conf->outputs + conf->num_outputs,
                [] (MirDisplayOutput const& output)
                {
                    delete[] output.modes;
                    delete[] output.output_formats;
                });

            delete[] conf->outputs;
            delete[] conf->cards;
            delete conf;
        });
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
    output->current_format = NoOfFormats > 0 ? formats[0] : mir_pixel_format_invalid;
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

}

std::shared_ptr<MirDisplayConfiguration> mt::build_trivial_configuration()
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
        default_physical_height_mm,
        mir_power_mode_on,
        mir_orientation_normal
    }};

    init_output(outputs, modes, formats);

    return build_test_config(outputs, cards);
}

std::shared_ptr<MirDisplayConfiguration> mt::build_non_trivial_configuration()
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
            default_physical_height_mm,
            mir_power_mode_on,
            mir_orientation_normal
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
            default_physical_height_mm,
            mir_power_mode_on,
            mir_orientation_normal
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
            default_physical_height_mm,
            mir_power_mode_on,
            mir_orientation_normal
        },
    };

    init_outputs(outputs, modes, formats);

    return build_test_config(outputs, cards);
}


