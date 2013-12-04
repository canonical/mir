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

#include "android_display_configuration.h"

namespace mg = mir::graphics;
namespace mga = mg::android;
namespace geom = mir::geometry;

mga::AndroidDisplayConfiguration::AndroidDisplayConfiguration(geom::Size const& display_size)
        : configuration{mg::DisplayConfigurationOutputId{1},
                        mg::DisplayConfigurationCardId{0},
                        mg::DisplayConfigurationOutputType::lvds,
                        {mir_pixel_format_abgr_8888},
                        {mg::DisplayConfigurationMode{display_size,0.0f}},
                        0,
                        geom::Size{0,0},
                        true,
                        true,
                        geom::Point{0,0},
                        0, 0, mir_power_mode_on},
          card{mg::DisplayConfigurationCardId{0}, 1}
{
}

mga::AndroidDisplayConfiguration::AndroidDisplayConfiguration(AndroidDisplayConfiguration const& other)
    : DisplayConfiguration(), configuration(other.configuration),
      card(other.card)
{
}

mga::AndroidDisplayConfiguration& mga::AndroidDisplayConfiguration::operator=(AndroidDisplayConfiguration const& other)
{
    if (&other != this)
    {
        configuration = other.configuration;
        card = other.card;
    }
    return *this;
}

void mga::AndroidDisplayConfiguration::for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mga::AndroidDisplayConfiguration::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    f(configuration);
}

void mga::AndroidDisplayConfiguration::configure_output(mg::DisplayConfigurationOutputId, bool, geom::Point, size_t, MirPowerMode power_mode)
{
    configuration.power_mode = power_mode;
}
