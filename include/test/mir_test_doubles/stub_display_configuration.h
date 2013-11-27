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
#include "mir/geometry/rectangle.h"
#include "mir_toolkit/common.h"

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
        : StubDisplayConfig(num_displays,
                            {
                                mir_pixel_format_bgr_888,
                                mir_pixel_format_abgr_8888,
                                mir_pixel_format_xbgr_8888
                            })
    {
    }

    StubDisplayConfig(unsigned int num_displays,
                      std::vector<MirPixelFormat> const& pfs)
    {
        /* construct a non-trivial dummy display config to send */
        int mode_index = 0;
        for (auto i = 0u; i < num_displays; i++)
        {
            std::vector<graphics::DisplayConfigurationMode> modes;

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
                graphics::DisplayConfigurationOutputId{static_cast<int>(i + 1)},
                graphics::DisplayConfigurationCardId{static_cast<int>(i)},
                graphics::DisplayConfigurationOutputType::vga,
                pfs, modes, i,
                physical_size,
                ((i % 2) == 0),
                ((i % 2) == 1),
                top_left,
                mode_index, 1u,
                mir_power_mode_off
            };

            outputs.push_back(output);

            graphics::DisplayConfigurationCard card{
                graphics::DisplayConfigurationCardId{static_cast<int>(i)},
                i + 1
            };

            cards.push_back(card);
        }

    };

    StubDisplayConfig(std::vector<geometry::Rectangle> const& rects)
    {
        int id = 1;
        for (auto const& rect : rects)
        {
            graphics::DisplayConfigurationOutput output
            {
                graphics::DisplayConfigurationOutputId{id},
                graphics::DisplayConfigurationCardId{0},
                graphics::DisplayConfigurationOutputType::vga,
                std::vector<MirPixelFormat>{mir_pixel_format_abgr_8888},
                {{rect.size, 60.0}},
                0, geometry::Size{}, true, true, rect.top_left, 0, 0, mir_power_mode_on
            };

            outputs.push_back(output);
            ++id;
        }

        graphics::DisplayConfigurationCard card{
            graphics::DisplayConfigurationCardId{static_cast<int>(1)},
            rects.size()
        };

        cards.push_back(card);
    }

    void for_each_card(std::function<void(graphics::DisplayConfigurationCard const&)> f) const
    {
        for (auto const& card : cards)
            f(card);
    }

    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const
    {
        for (auto& disp : outputs)
        {
            f(disp);
        }
    }

    void configure_output(graphics::DisplayConfigurationOutputId, bool, geometry::Point, size_t, MirPowerMode)
    {
    }

    std::vector<graphics::DisplayConfigurationCard> cards;
    std::vector<graphics::DisplayConfigurationOutput> outputs;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_ */
