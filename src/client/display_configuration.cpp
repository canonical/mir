/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "display_configuration.h"

#include <cstring>

namespace mcl = mir::client;
namespace mp = mir::protobuf;

void mcl::delete_config_storage(MirDisplayConfiguration* config)
{
    if (!config)
        return;

    for(auto i=0u; i< config->num_displays; i++)
    {
        if (config->displays[i].modes)
            ::operator delete(config->displays[i].modes);
        if (config->displays[i].output_formats)
            ::operator delete(config->displays[i].output_formats);
    }
    if (config->displays)
        ::operator delete(config->displays);

    ::operator delete(config);
}

namespace
{
void fill_display_output(MirDisplayOutput& output, mp::DisplayOutput const& msg)
{
    output.card_id = msg.card_id();
    output.output_id = msg.output_id();

    for(auto i=0u; i < output.num_modes; i++)
    {
        auto mode = msg.mode(i);
        output.modes[i].horizontal_resolution = mode.horizontal_resolution(); 
        output.modes[i].vertical_resolution = mode.vertical_resolution(); 
        output.modes[i].refresh_rate = mode.refresh_rate();
    }
    output.current_mode = msg.current_mode();

    for(auto i=0u; i < output.num_output_formats; i++)
    {
        output.output_formats[i] = static_cast<MirPixelFormat>(msg.pixel_format(i));
    }
    output.current_output_format = msg.current_format();

    output.position_x = msg.position_x();
    output.position_y = msg.position_y();
    output.connected = msg.connected();
    output.used = msg.used();
    output.physical_width_mm = msg.physical_width_mm();
    output.physical_height_mm = msg.physical_height_mm();
}
}

MirDisplayConfiguration* mcl::set_display_config_from_message(mp::Connection const& connection_msg)
{
    auto config = static_cast<MirDisplayConfiguration*>(::operator new(sizeof(MirDisplayConfiguration)));
    config->num_displays = connection_msg.display_output_size();
    config->displays = static_cast<MirDisplayOutput*>(::operator new(sizeof(MirDisplayOutput) * config->num_displays));

    for(auto i=0u; i < config->num_displays; i++)
    {
        auto output = connection_msg.display_output(i);
        config->displays[i].num_output_formats = output.pixel_format_size();
        config->displays[i].output_formats = static_cast<MirPixelFormat*>(
            ::operator new(sizeof(MirPixelFormat)*config->displays[i].num_output_formats));
 
        config->displays[i].num_modes = output.mode_size();
        config->displays[i].modes = static_cast<MirDisplayMode*>(
            ::operator new(sizeof(MirDisplayMode)*config->displays[i].num_modes));

        fill_display_output(config->displays[i], output);
    }
    return config;
}
