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
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_
#include "mir/graphics/display_configuration.h"
#include <vector>

namespace mir
{
namespace test
{
namespace doubles
{

class StubDisplayConfig : public graphics::DisplayConfiguration
{
public:
    StubDisplayConfig()
        : StubDisplayConfig(3)
    {
    }

    StubDisplayConfig(unsigned int num_displays)
    {
        /* construct a non-trivial dummy display config to send */
        int mode_index = 0;
        for (auto i = 0u; i < num_displays; i++)
        {
            std::vector<graphics::DisplayConfigurationMode> modes;
            std::vector<geometry::PixelFormat> pfs{
                geometry::PixelFormat::bgr_888,
                geometry::PixelFormat::abgr_8888,
                geometry::PixelFormat::xbgr_8888};

            for (auto j = 0u; j <= i; j++)
            {
                geometry::Size sz{mode_index*4, mode_index*3};
                graphics::DisplayConfigurationMode mode{sz, 10.0f * mode_index };
                mode_index++;
                modes.push_back(mode);
            }

            size_t mode_index = modes.size() - 1; 
            geometry::Size physical_size{};
            geometry::Point top_left{};
            graphics::DisplayConfigurationOutput output{
                graphics::DisplayConfigurationOutputId{static_cast<int>(i)},
                graphics::DisplayConfigurationCardId{static_cast<int>(i)},
                pfs, modes,
                physical_size,
                ((i % 2) == 0),
                ((i % 2) == 1),
                top_left,
                mode_index, 1u
            };

            outputs.push_back(output);
        }
    };

    void for_each_card(std::function<void(graphics::DisplayConfigurationCard const&)>) const
    {
    }

    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const
    {
        for (auto& disp : outputs)
        {
            f(disp);
        }
    }

    void configure_output(graphics::DisplayConfigurationOutputId, bool, geometry::Point, size_t)
    {
    }

    std::vector<graphics::DisplayConfigurationOutput> outputs;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_ */
