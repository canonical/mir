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
#include "src/client/display_configuration.h"

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
MirOutputConnectionState const default_connected = mir_output_connection_state_connected;
uint32_t const default_used = 1;
uint32_t const default_physical_width_mm = 0;
uint32_t const default_physical_height_mm = 0;

}

std::shared_ptr<MirDisplayConfig> mt::build_trivial_configuration()
{
    mir::protobuf::DisplayConfiguration conf;

    auto output = conf.add_display_output();
    auto mode   = output->add_mode();
    mode->set_horizontal_resolution(1080);
    mode->set_vertical_resolution(1920);
    mode->set_refresh_rate(4.33f);

    output->add_pixel_format(mir_pixel_format_abgr_8888);
    output->set_current_format(default_current_output_format);
    output->set_current_mode(default_current_mode);
    output->set_position_x(default_position_x);
    output->set_position_y(default_position_y);
    output->set_output_id(default_output_id);
    output->set_connected(default_connected);
    output->set_used(default_used);
    output->set_physical_width_mm(default_physical_width_mm);
    output->set_physical_height_mm(default_physical_height_mm);
    output->set_type(default_type);
    output->set_preferred_mode(default_preferred_mode);
    output->set_power_mode(mir_power_mode_on);
    output->set_orientation(mir_orientation_normal);

    return std::make_shared<MirDisplayConfig>(conf);
}

std::shared_ptr<MirDisplayConfig> mt::build_non_trivial_configuration()
{
    mir::protobuf::DisplayConfiguration conf;

    for (int i = 0; i < 3; i++)
    {
        auto output = conf.add_display_output();
        auto mode   = output->add_mode();
        mode->set_horizontal_resolution(1080);
        mode->set_vertical_resolution(1920);
        mode->set_refresh_rate(4.33f);

        mode = output->add_mode();
        mode->set_horizontal_resolution(1080);
        mode->set_vertical_resolution(1920);
        mode->set_refresh_rate(1.11f);

        output->add_pixel_format(mir_pixel_format_abgr_8888);
        output->add_pixel_format(mir_pixel_format_xbgr_8888);
        output->add_pixel_format(mir_pixel_format_argb_8888);
        output->add_pixel_format(mir_pixel_format_xrgb_8888);
        output->add_pixel_format(mir_pixel_format_bgr_888);
        output->set_current_format(default_current_output_format);
        output->set_current_mode(default_current_mode);
        output->set_position_x(default_position_x);
        output->set_position_y(default_position_y);
        output->set_output_id(default_output_id);
        output->set_connected(default_connected);
        output->set_used(default_used);
        output->set_physical_width_mm(default_physical_width_mm);
        output->set_physical_height_mm(default_physical_height_mm);
        output->set_type(default_type);
        output->set_preferred_mode(default_preferred_mode);
        output->set_power_mode(mir_power_mode_on);
        output->set_orientation(mir_orientation_normal);
    }

    return std::make_shared<MirDisplayConfig>(conf);
}
