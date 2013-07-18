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

mcl::DisplayConfiguration::DisplayConfiguration()
    : config{0, nullptr}
{
}

void mcl::delete_config_storage(MirDisplayConfiguration& config)
{
    for(auto i=0u; i< config.num_displays; i++)
    {
        if (config.displays[i].modes)
            ::operator delete (config.displays[i].modes);
        if (config.displays[i].pixel_formats)
            ::operator delete (config.displays[i].pixel_formats);
    }
    if (config.displays)
        ::operator delete (config.displays);

    config.num_displays = 0;
}

namespace
{
void fill_display_info(MirDisplayState& info, mp::DisplayInfo const& msg)
{
    info.card_id = msg.card_id();
    info.output_id = msg.output_id();

    for(auto i=0u; i < info.num_modes; i++)
    {
        auto mode = msg.mode(i);
        info.modes[i].horizontal_resolution = mode.horizontal_resolution(); 
        info.modes[i].vertical_resolution = mode.vertical_resolution(); 
        info.modes[i].refresh_rate = mode.refresh_rate();
    }
    info.current_mode = msg.current_mode();

    for(auto i=0u; i < info.num_pixel_formats; i++)
    {
        info.pixel_formats[i] = (MirPixelFormat) msg.pixel_format(i);
    }
    info.current_format = msg.current_format();

    info.position_x = msg.position_x();
    info.position_y = msg.position_y();
    info.connected = msg.connected();
    info.used = msg.used();
    info.physical_width_mm = msg.physical_width_mm();
    info.physical_height_mm = msg.physical_height_mm();
}

void alloc_storage_from_msg(MirDisplayConfiguration& config, mp::Connection& msg)
{
    config.num_displays = msg.display_info_size();
    config.displays = static_cast<MirDisplayState*>(::operator new(sizeof(MirDisplayState) * config.num_displays));

    for(auto i=0u; i < config.num_displays; i++)
    {
        auto info = msg.display_info(i);
        config.displays[i].num_pixel_formats = info.pixel_format_size();
        config.displays[i].pixel_formats = static_cast<MirPixelFormat*>(
            ::operator new(sizeof(MirPixelFormat)*config.displays[i].num_pixel_formats));
 
        config.displays[i].num_modes = info.mode_size();
        config.displays[i].modes = static_cast<MirDisplayMode*>(
            ::operator new(sizeof(MirDisplayMode)*config.displays[i].num_modes));
    }
}
}

mcl::DisplayConfiguration::~DisplayConfiguration()
{
    mcl::delete_config_storage(config);
}

mcl::DisplayConfiguration::operator MirDisplayConfiguration&()
{
    return config;
}

void mcl::DisplayConfiguration::set_from_message(protobuf::Connection& connection_msg)
{
    mcl::delete_config_storage(config);
    alloc_storage_from_msg(config, connection_msg);

    for(auto i=0u; i < config.num_displays; i++)
    {
        fill_display_info(config.displays[i], connection_msg.display_info(i));
    }
}
