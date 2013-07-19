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
void fill_display_state(MirDisplayState& state, mp::DisplayState const& msg)
{
    state.card_id = msg.card_id();
    state.output_id = msg.output_id();

    for(auto i=0u; i < state.num_modes; i++)
    {
        auto mode = msg.mode(i);
        state.modes[i].horizontal_resolution = mode.horizontal_resolution(); 
        state.modes[i].vertical_resolution = mode.vertical_resolution(); 
        state.modes[i].refresh_rate = mode.refresh_rate();
    }
    state.current_mode = msg.current_mode();

    for(auto i=0u; i < state.num_pixel_formats; i++)
    {
        state.pixel_formats[i] = (MirPixelFormat) msg.pixel_format(i);
    }
    state.current_format = msg.current_format();

    state.position_x = msg.position_x();
    state.position_y = msg.position_y();
    state.connected = msg.connected();
    state.used = msg.used();
    state.physical_width_mm = msg.physical_width_mm();
    state.physical_height_mm = msg.physical_height_mm();
}
}

void mcl::set_display_config_from_message(MirDisplayConfiguration& config, mp::Connection const& connection_msg)
{
    config.num_displays = connection_msg.display_state_size();
    config.displays = static_cast<MirDisplayState*>(::operator new(sizeof(MirDisplayState) * config.num_displays));

    for(auto i=0u; i < config.num_displays; i++)
    {
        auto state = connection_msg.display_state(i);
        config.displays[i].num_pixel_formats = state.pixel_format_size();
        config.displays[i].pixel_formats = static_cast<MirPixelFormat*>(
            ::operator new(sizeof(MirPixelFormat)*config.displays[i].num_pixel_formats));
 
        config.displays[i].num_modes = state.mode_size();
        config.displays[i].modes = static_cast<MirDisplayMode*>(
            ::operator new(sizeof(MirDisplayMode)*config.displays[i].num_modes));

        fill_display_state(config.displays[i], state);
    }
}
