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

#ifndef MIR_GRAPHICS_OFFSCREEN_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_OFFSCREEN_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"

namespace mir
{
namespace graphics
{
namespace offscreen
{

class DisplayConfiguration : public graphics::DisplayConfiguration
{
public:
    DisplayConfiguration(geometry::Size const& display_size);
    DisplayConfiguration(DisplayConfiguration const& other);
    DisplayConfiguration& operator=(DisplayConfiguration const& other);

    void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) const override;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f) override;

private:
    DisplayConfigurationOutput output;
    DisplayConfigurationCard card;
};


}
}
}

#endif /* MIR_GRAPHICS_OFFSCREEN_DISPLAY_CONFIGURATION_H_ */
