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
 */

#include "display_configuration.h"
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mga = mg::android;
namespace geom = mir::geometry;

namespace
{
enum DisplayIds
{
    primary_id,
    external_id,
    max_displays
};
}

mga::DisplayConfiguration::DisplayConfiguration(
    mg::DisplayConfigurationOutput primary_config,
    MirPowerMode primary_mode,
    mg::DisplayConfigurationOutput external_config,
    MirPowerMode external_mode) :
    configurations{
        {std::move(primary_config),
        std::move(external_config)}
    },
    card{mg::DisplayConfigurationCardId{0}, max_displays}
{
    primary().power_mode = primary_mode;
    external().power_mode = external_mode;
}

mga::DisplayConfiguration::DisplayConfiguration(DisplayConfiguration const& other) :
    mg::DisplayConfiguration(),
    configurations(other.configurations),
    card(other.card)
{
}

mga::DisplayConfiguration& mga::DisplayConfiguration::operator=(DisplayConfiguration const& other)
{
    if (&other != this)
    {
        configurations = other.configurations;
        card = other.card;
    }
    return *this;
}

void mga::DisplayConfiguration::for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mga::DisplayConfiguration::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    for (auto const& configuration : configurations)
        f(configuration);
}

void mga::DisplayConfiguration::for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f)
{
    for (auto& configuration : configurations)
    {
        mg::UserDisplayConfigurationOutput user(configuration);
        f(user);
    }
}

mg::DisplayConfigurationOutput& mga::DisplayConfiguration::primary()
{
    return configurations[primary_id];
}

mg::DisplayConfigurationOutput& mga::DisplayConfiguration::external()
{
    return configurations[external_id];
}

mg::DisplayConfigurationOutput& mga::DisplayConfiguration::operator[](mg::DisplayConfigurationOutputId const& disp_id)
{
    auto id = disp_id.as_value();
    if (id != primary_id && id != external_id)
        BOOST_THROW_EXCEPTION(std::invalid_argument("invalid display id"));
    return configurations[id];
}
