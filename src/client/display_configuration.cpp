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
            delete[] config->displays[i].modes;
        if (config->displays[i].output_formats)
            delete[] config->displays[i].output_formats;
    }
    if (config->displays)
        delete[] config->displays;

    delete config;
}

mcl::DisplayOutput::DisplayOutput(size_t num_modes_, size_t num_formats)
{
    num_modes = num_modes_;
    modes = new MirDisplayMode[num_modes];
   
    num_output_formats = num_formats; 
    output_formats = new MirPixelFormat[num_formats];
}

mcl::DisplayOutput::~DisplayOutput()
{
    delete[] modes;
    delete[] output_formats;
}

namespace
{

void fill_display_card(MirDisplayCard& card, mp::DisplayCard const& msg)
{
    card.card_id = msg.card_id();
    card.max_simultaneous_outputs = msg.max_simultaneous_outputs();
}

void fill_display_output(MirDisplayOutput& output, mp::DisplayOutput const& msg)
{
    output.card_id = msg.card_id();
    output.output_id = msg.output_id();
    output.type = static_cast<MirDisplayOutputType>(msg.type());

    for (auto i = 0u; i < output.num_modes; i++)
    {
        auto mode = msg.mode(i);
        output.modes[i].horizontal_resolution = mode.horizontal_resolution(); 
        output.modes[i].vertical_resolution = mode.vertical_resolution(); 
        output.modes[i].refresh_rate = mode.refresh_rate();
    }
    output.preferred_mode = msg.preferred_mode();
    output.current_mode = msg.current_mode();

    for (auto i = 0u; i < output.num_output_formats; i++)
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

mcl::DisplayConfiguration::DisplayConfiguration()
    : notify_change([]{})
{
}

mcl::DisplayConfiguration::~DisplayConfiguration()
{
}

void mcl::DisplayConfiguration::set_configuration(mp::DisplayConfiguration const& msg)
{
    std::unique_lock<std::mutex> lk(guard);

    cards.clear();
    for (auto i = 0; i < msg.display_card_size(); i++)
    {
        auto const& msg_card = msg.display_card(i);
        MirDisplayCard card;
        fill_display_card(card, msg_card);
        cards.push_back(card);
    }

    outputs.clear();
    for (auto i = 0; i < msg.display_output_size(); i++)
    {
        auto const& msg_output = msg.display_output(i);
        auto output = std::make_shared<mcl::DisplayOutput>(msg_output.mode_size(), msg_output.pixel_format_size());
        fill_display_output(*output, msg_output);
        outputs.push_back(output);
    }
}

void mcl::DisplayConfiguration::update_configuration(mp::DisplayConfiguration const& msg)
{
    set_configuration(msg);

    notify_change();
}

//user is responsible for freeing the returned value
MirDisplayConfiguration* mcl::DisplayConfiguration::copy_to_client() const
{
    std::unique_lock<std::mutex> lk(guard);
    auto new_config = new MirDisplayConfiguration;

    /* Cards */
    new_config->num_cards = cards.size();
    new_config->cards = new MirDisplayCard[new_config->num_cards];

    for (auto i = 0u; i < cards.size(); i++)
        new_config->cards[i] = cards[i];

    /* Outputs */
    new_config->num_displays = outputs.size();
    new_config->displays = new MirDisplayOutput[new_config->num_displays];

    for (auto i = 0u; i < outputs.size(); i++)
    {
        auto new_info = &new_config->displays[i];
        MirDisplayOutput* output = outputs[i].get();
        std::memcpy(new_info, output, sizeof(MirDisplayOutput)); 

        new_info->output_formats = new MirPixelFormat[new_info->num_output_formats];
        auto format_size = sizeof(MirPixelFormat) * new_info->num_output_formats;
        std::memcpy(new_info->output_formats, output->output_formats, format_size);

        new_info->modes = new MirDisplayMode[new_info->num_modes];
        auto mode_size = sizeof(MirDisplayMode)* new_info->num_modes;
        std::memcpy(new_info->modes, output->modes, mode_size);
    }

    return new_config;
}

void mcl::DisplayConfiguration::set_display_change_handler(std::function<void()> const& fn)
{
    std::unique_lock<std::mutex> lk(guard);
    notify_change = fn;
}
