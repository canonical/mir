/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "display_configuration.h"

namespace mg = mir::graphics;
namespace mgo = mg::offscreen;
namespace geom = mir::geometry;

mgo::DisplayConfiguration::DisplayConfiguration(geom::Size const& display_size)
        : output{mg::DisplayConfigurationOutputId{1},
                 mg::DisplayConfigurationCardId{0},
                 mg::DisplayConfigurationOutputType::lvds,
                 {mir_pixel_format_xrgb_8888},
                 {mg::DisplayConfigurationMode{display_size,0.0f}},
                 0,
                 geom::Size{0,0},
                 true,
                 true,
                 geom::Point{0,0},
                 0,
                 mir_pixel_format_xrgb_8888,
                 mir_power_mode_on,
                 mir_orientation_normal,
                 1.0f,
                 mir_form_factor_monitor},
          card{mg::DisplayConfigurationCardId{0}, 1}
{
}

mgo::DisplayConfiguration::DisplayConfiguration(DisplayConfiguration const& other)
    : mg::DisplayConfiguration(),
      output(other.output),
      card(other.card)
{
}

mgo::DisplayConfiguration&
mgo::DisplayConfiguration::operator=(DisplayConfiguration const& other)
{
    if (&other != this)
    {
        output = other.output;
        card = other.card;
    }
    return *this;
}

void mgo::DisplayConfiguration::for_each_card(
    std::function<void(mg::DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mgo::DisplayConfiguration::for_each_output(
    std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    f(output);
}

void mgo::DisplayConfiguration::for_each_output(
    std::function<void(mg::UserDisplayConfigurationOutput&)> f)
{
    mg::UserDisplayConfigurationOutput user(output);
    f(user);
}

