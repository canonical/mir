/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "translucent_server_configuration.h"

#include "pixel_format_selector.h"

namespace mir
{
namespace examples
{

TranslucentServerConfiguration::TranslucentServerConfiguration(int argc, char const** argv)
    : BasicServerConfiguration(argc, argv)
{
}

std::shared_ptr<graphics::DisplayConfigurationPolicy>
TranslucentServerConfiguration::the_display_configuration_policy()
{
    return display_configuration_policy(
        [this]() -> std::shared_ptr<graphics::DisplayConfigurationPolicy>
        {
            return std::make_shared<PixelFormatSelector>(BasicServerConfiguration::the_display_configuration_policy(), true);
        });
}


}
}
