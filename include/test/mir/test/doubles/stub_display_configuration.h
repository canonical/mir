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

struct StubDisplayConfigurationOutput : public graphics::DisplayConfigurationOutput
{
    StubDisplayConfigurationOutput(
        geometry::Size px_size, geometry::Size mm_size, MirPixelFormat format, double vrefresh, bool connected) :
        StubDisplayConfigurationOutput(graphics::DisplayConfigurationOutputId{0}, px_size, mm_size, format, vrefresh, connected)
    {
    }

    StubDisplayConfigurationOutput(graphics::DisplayConfigurationOutputId id,
        geometry::Size px_size, geometry::Size mm_size, MirPixelFormat format, double vrefresh, bool connected) :
        DisplayConfigurationOutput{
            id,
            graphics::DisplayConfigurationCardId{0},
            graphics::DisplayConfigurationOutputType::lvds,
            {format},
            {{px_size, vrefresh}},
            0,
            mm_size,
            connected,
            connected,
            {0,0},
            0,
            format,
            mir_power_mode_on,
            mir_orientation_normal,
            1.0f,
            mir_form_factor_monitor
        }
    {
    }
};

class StubDisplayConfig : public graphics::DisplayConfiguration
{
public:
    StubDisplayConfig()
        : StubDisplayConfig(3)
    {
    }

    StubDisplayConfig(StubDisplayConfig const& other)
        : graphics::DisplayConfiguration(),
          cards(other.cards),
          outputs(other.outputs)
    {
    }

    StubDisplayConfig(graphics::DisplayConfiguration const& other)
        : graphics::DisplayConfiguration()
    {
        other.for_each_card([this](auto card) { cards.push_back(card); });
        other.for_each_output(
            [this](graphics::DisplayConfigurationOutput const& output)
            {
                outputs.push_back(output);
            });
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

    StubDisplayConfig(std::vector<std::pair<bool,bool>> const& connected_used)
        : StubDisplayConfig(connected_used.size())
    {
        for (auto i = 0u; i < outputs.size(); ++i)
        {
            outputs[i].connected = connected_used[i].first;
            outputs[i].used = connected_used[i].second;
            outputs[i].current_format = mir_pixel_format_abgr_8888;
            outputs[i].id = graphics::DisplayConfigurationOutputId{static_cast<int>(i)};
        }
    }

    StubDisplayConfig(unsigned int num_displays,
                      std::vector<MirPixelFormat> const& pfs)
    {
        /* construct a non-trivial dummy display config to send */
        int mode_index = 0;
        for (auto i = 0u; i < num_displays; i++)
        {
            std::vector<graphics::DisplayConfigurationMode> modes;

            // Every second output, starting with the first, is connected...
            // (Android tests assume the first output in a configuration is connected)
            auto const connected = [](int index) { return (index % 2) == 0; };
            // ..and every second connected output is used...
            auto const used = [](int index) { return (index % 4) == 0; };

            for (auto j = 0u; j <= i; j++)
            {
                geometry::Size sz{mode_index*4, mode_index*3};
                graphics::DisplayConfigurationMode mode{sz, 10.0f * mode_index };
                mode_index++;
                modes.push_back(mode);
            }

            uint32_t current_mode_index;
            uint32_t preferred_mode_index;
            if (connected(i))
            {
                current_mode_index = modes.size() - 1;
                preferred_mode_index = i;
            }
            else
            {
                current_mode_index = std::numeric_limits<uint32_t>::max();
                preferred_mode_index = std::numeric_limits<uint32_t>::max();
            }

            geometry::Size physical_size{};
            geometry::Point top_left{};
            graphics::DisplayConfigurationOutput output{
                graphics::DisplayConfigurationOutputId{static_cast<int>(i + 1)},
                graphics::DisplayConfigurationCardId{static_cast<int>(i)},
                graphics::DisplayConfigurationOutputType::vga,
                pfs,
                connected(i) ? modes : std::vector<graphics::DisplayConfigurationMode>{},
                preferred_mode_index,
                physical_size,
                connected(i),
                used(i),
                top_left,
                current_mode_index, pfs[0],
                used(i) ? mir_power_mode_on : mir_power_mode_off,
                mir_orientation_normal,
                1.0f,
                mir_form_factor_monitor
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
                graphics::DisplayConfigurationCardId{1},
                graphics::DisplayConfigurationOutputType::vga,
                std::vector<MirPixelFormat>{mir_pixel_format_abgr_8888},
                {{rect.size, 60.0}},
                0, geometry::Size{}, true, true, rect.top_left, 0,
                mir_pixel_format_abgr_8888, mir_power_mode_on,
                mir_orientation_normal,
                1.0f,
                mir_form_factor_monitor
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

    StubDisplayConfig(std::vector<graphics::DisplayConfigurationOutput> const& outputs)
    {
        graphics::DisplayConfigurationCard card{
            graphics::DisplayConfigurationCardId{static_cast<int>(1)},
            outputs.size()
        };

        cards.push_back(card);
        this->outputs = outputs;

        for (auto& output : this->outputs)
        {
            output.card_id = cards[0].id;
        }
    }

    void for_each_card(std::function<void(graphics::DisplayConfigurationCard const&)> f) const override
    {
        for (auto const& card : cards)
            f(card);
    }

    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const override
    {
        for (auto& disp : outputs)
        {
            f(disp);
        }
    }

    void for_each_output(std::function<void(graphics::UserDisplayConfigurationOutput&)> f) override
    {
        for (auto& disp : outputs)
        {
            graphics::UserDisplayConfigurationOutput user(disp);
            f(user);
        }
    }

    std::vector<graphics::DisplayConfigurationCard> cards;
    std::vector<graphics::DisplayConfigurationOutput> outputs;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_ */
