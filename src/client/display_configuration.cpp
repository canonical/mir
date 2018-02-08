/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

    for(auto i=0u; i< config->num_outputs; i++)
    {
        if (config->outputs[i].modes)
            delete[] config->outputs[i].modes;
        if (config->outputs[i].output_formats)
            delete[] config->outputs[i].output_formats;
    }

    if (config->outputs)
        delete[] config->outputs;
    if (config->cards)
        delete[] config->cards;

    delete config;
}

mcl::DisplayOutput::DisplayOutput(size_t num_modes_, size_t num_formats)
{
    num_modes = num_modes_;
    modes = new MirDisplayMode[num_modes];

    num_output_formats = num_formats;
    output_formats = new MirPixelFormat[num_formats];
}

mcl::DisplayOutput::DisplayOutput(DisplayOutput&& rhs)
{
    std::memcpy(
        static_cast<MirDisplayOutput*>(this),
        static_cast<MirDisplayOutput*>(&rhs),
        sizeof(MirDisplayOutput));

    rhs.modes = nullptr;
    rhs.output_formats = nullptr;
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

    output.num_modes = msg.mode_size();
    output.modes = new MirDisplayMode[output.num_modes];
    for (auto i = 0u; i < output.num_modes; i++)
    {
        auto mode = msg.mode(i);
        output.modes[i].horizontal_resolution = mode.horizontal_resolution();
        output.modes[i].vertical_resolution = mode.vertical_resolution();
        output.modes[i].refresh_rate = mode.refresh_rate();
    }
    output.preferred_mode = msg.preferred_mode();
    output.current_mode = msg.current_mode();

    output.num_output_formats = msg.pixel_format_size();
    output.output_formats = new MirPixelFormat[output.num_output_formats];
    for (auto i = 0u; i < output.num_output_formats; i++)
    {
        output.output_formats[i] = static_cast<MirPixelFormat>(msg.pixel_format(i));
    }
    output.current_format = static_cast<MirPixelFormat>(msg.current_format());

    output.position_x = msg.position_x();
    output.position_y = msg.position_y();
    output.connected = msg.connected();
    output.used = msg.used();
    output.physical_width_mm = msg.physical_width_mm();
    output.physical_height_mm = msg.physical_height_mm();
    output.power_mode = static_cast<MirPowerMode>(msg.power_mode());
    output.orientation = static_cast<MirOrientation>(msg.orientation());
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
    std::lock_guard<std::mutex> lk{guard};
    config = msg;
}

void mcl::DisplayConfiguration::update_configuration(mp::DisplayConfiguration const& msg)
{
    set_configuration(msg);

    notify_change();
}

//user is responsible for freeing the returned value
MirDisplayConfiguration* mcl::DisplayConfiguration::copy_to_client() const
{
    std::lock_guard<std::mutex> lock{guard};

    auto new_config = new MirDisplayConfiguration;

    new_config->num_cards = config.display_card_size();
    new_config->cards = new MirDisplayCard[new_config->num_cards];

    for (auto i = 0; i < config.display_card_size(); i++)
    {
        auto const& msg_card = config.display_card(i);
        fill_display_card(new_config->cards[i], msg_card);
    }

    /* Outputs */
    new_config->num_outputs = config.display_output_size();
    new_config->outputs = new MirDisplayOutput[new_config->num_outputs];
    for (auto i = 0; i < config.display_output_size(); i++)
    {
        auto const& msg_output = config.display_output(i);
        fill_display_output(new_config->outputs[i], msg_output);
    }

    return new_config;
}

auto mcl::DisplayConfiguration::take_snapshot() const -> std::unique_ptr<protobuf::DisplayConfiguration>
{
    std::lock_guard<std::mutex> lk{guard};
    return std::make_unique<protobuf::DisplayConfiguration>(config);
}

void mcl::DisplayConfiguration::set_display_change_handler(std::function<void()> const& fn)
{
    std::lock_guard<std::mutex> lk(guard);
    notify_change = fn;
}
