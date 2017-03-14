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

uint32_t const default_preferred_mode = 0;
uint32_t const default_current_mode = 0;
MirPixelFormat const default_current_output_format = mir_pixel_format_abgr_8888;
uint32_t const default_output_id = 0;
auto const default_type = MirDisplayOutputType(0);
int32_t const default_position_x = 0;
int32_t const default_position_y = 0;
MirOutputConnectionState const default_connected = mir_output_connection_state_connected;
uint32_t const default_used = 1;
uint32_t const default_physical_width_mm = 0;
uint32_t const default_physical_height_mm = 0;

unsigned char const valid_edid[] =
    "\x00\xff\xff\xff\xff\xff\xff\x00\x10\xac\x46\xf0\x4c\x4a\x31\x41"
    "\x05\x19\x01\x04\xb5\x34\x20\x78\x3a\x1d\xf5\xae\x4f\x35\xb3\x25"
    "\x0d\x50\x54\xa5\x4b\x00\x81\x80\xa9\x40\xd1\x00\x71\x4f\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x28\x3c\x80\xa0\x70\xb0\x23\x40\x30\x20"
    "\x36\x00\x06\x44\x21\x00\x00\x1a\x00\x00\x00\xff\x00\x59\x43\x4d"
    "\x30\x46\x35\x31\x52\x41\x31\x4a\x4c\x0a\x00\x00\x00\xfc\x00\x44"
    "\x45\x4c\x4c\x20\x55\x32\x34\x31\x33\x0a\x20\x20\x00\x00\x00\xfd"
    "\x00\x38\x4c\x1e\x51\x11\x00\x0a\x20\x20\x20\x20\x20\x20\x01\x42"
    "\x02\x03\x1d\xf1\x50\x90\x05\x04\x03\x02\x07\x16\x01\x1f\x12\x13"
    "\x14\x20\x15\x11\x06\x23\x09\x1f\x07\x83\x01\x00\x00\x02\x3a\x80"
    "\x18\x71\x38\x2d\x40\x58\x2c\x45\x00\x06\x44\x21\x00\x00\x1e\x01"
    "\x1d\x80\x18\x71\x1c\x16\x20\x58\x2c\x25\x00\x06\x44\x21\x00\x00"
    "\x9e\x01\x1d\x00\x72\x51\xd0\x1e\x20\x6e\x28\x55\x00\x06\x44\x21"
    "\x00\x00\x1e\x8c\x0a\xd0\x8a\x20\xe0\x2d\x10\x10\x3e\x96\x00\x06"
    "\x44\x21\x00\x00\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x09";

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
    output->set_edid(valid_edid, sizeof(valid_edid));
    output->set_subpixel_arrangement(mir_subpixel_arrangement_horizontal_rgb);

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
        output->set_edid(valid_edid, sizeof(valid_edid));
        output->set_subpixel_arrangement(mir_subpixel_arrangement_horizontal_rgb);
    }

    return std::make_shared<MirDisplayConfig>(conf);
}
