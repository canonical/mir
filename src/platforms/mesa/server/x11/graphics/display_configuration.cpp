/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "display_configuration.h"
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace geom = mir::geometry;

mgx::DisplayConfiguration::DisplayConfiguration(MirPixelFormat pf, geom::Size const size, MirOrientation orientation) :
    configuration{
            mg::DisplayConfigurationOutputId{1},
            mg::DisplayConfigurationCardId{0},
            mg::DisplayConfigurationOutputType::unknown,
            {pf},
            //TODO: query fps
            {mg::DisplayConfigurationMode{size, 60.0}},
            0,
            //TODO: query mm-size
            size,
            true,
            true,
            geom::Point{0, 0},
            0,
            pf,
            mir_power_mode_on,
            orientation},
    card{mg::DisplayConfigurationCardId{0}, 1}
{
}

void mgx::DisplayConfiguration::for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mgx::DisplayConfiguration::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    f(configuration);
}

void mgx::DisplayConfiguration::for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f)
{
    mg::UserDisplayConfigurationOutput user(configuration);
    f(user);
}
