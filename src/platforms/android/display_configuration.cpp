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

namespace mg = mir::graphics;
namespace mga = mg::android;
namespace geom = mir::geometry;

mga::DisplayConfiguration::DisplayConfiguration(
    mg::DisplayConfigurationOutput primary,
    mg::DisplayConfigurationOutput external) :
    configurations{std::move(primary), std::move(external)},
    card{mg::DisplayConfigurationCardId{0}, 1}
{
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
    for(auto const& configuration : configurations)
        f(configuration);
}

void mga::DisplayConfiguration::for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f)
{
    for(auto& configuration : configurations)
    {
        mg::UserDisplayConfigurationOutput user(configuration);
        f(user);
    }
}
