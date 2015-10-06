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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/display_config_matchers.h"
#include "mir/graphics/display_configuration.h"
#include "mir_protobuf.pb.h"
#include "mir_toolkit/client_types.h"
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;
namespace mt = mir::test;

namespace
{

class TestDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    TestDisplayConfiguration(mp::DisplayConfiguration const& protobuf_config)
    {
        /* Cards */
        for (int i = 0; i < protobuf_config.display_card_size(); i++)
        {
            auto const& protobuf_card = protobuf_config.display_card(i);
            mg::DisplayConfigurationCard display_card
            {
                mg::DisplayConfigurationCardId(protobuf_card.card_id()),
                protobuf_card.max_simultaneous_outputs(),
            };

            cards.push_back(display_card);
        }

        /* Outputs */
        for (int i = 0; i < protobuf_config.display_output_size(); i++)
        {
            auto const& protobuf_output = protobuf_config.display_output(i);
            mg::DisplayConfigurationOutput display_output
            {
                mg::DisplayConfigurationOutputId(protobuf_output.output_id()),
                mg::DisplayConfigurationCardId(protobuf_output.card_id()),
                static_cast<mg::DisplayConfigurationOutputType>(protobuf_output.type()),
                {},
                {},
                protobuf_output.preferred_mode(),
                geom::Size{protobuf_output.physical_width_mm(),
                           protobuf_output.physical_height_mm()},
                static_cast<bool>(protobuf_output.connected()),
                static_cast<bool>(protobuf_output.used()),
                geom::Point{protobuf_output.position_x(),
                            protobuf_output.position_y()},
                protobuf_output.current_mode(),
                static_cast<MirPixelFormat>(protobuf_output.current_format()),
                static_cast<MirPowerMode>(protobuf_output.power_mode()),
                static_cast<MirOrientation>(protobuf_output.orientation())
            };

            /* Modes */
            std::vector<mg::DisplayConfigurationMode> modes;
            for (int n = 0; n < protobuf_output.mode_size(); n++)
            {
                auto const& protobuf_mode = protobuf_output.mode(n);
                modes.push_back(
                    {
                        geom::Size{protobuf_mode.horizontal_resolution(),
                                   protobuf_mode.vertical_resolution()},
                        protobuf_mode.refresh_rate()
                    });
            }
            display_output.modes = modes;

            /* Pixel formats */
            std::vector<MirPixelFormat> pixel_formats;
            for (int n = 0; n < protobuf_output.pixel_format_size(); n++)
            {
                pixel_formats.push_back(
                    static_cast<MirPixelFormat>(protobuf_output.pixel_format(n)));
            }
            display_output.pixel_formats = pixel_formats;

            outputs.push_back(display_output);
        }
    }

    TestDisplayConfiguration(MirDisplayConfiguration const& client_config)
    {
        /* Cards */
        for (size_t i = 0; i < client_config.num_cards; i++)
        {
            auto const& client_card = client_config.cards[i];
            mg::DisplayConfigurationCard display_card
            {
                mg::DisplayConfigurationCardId(client_card.card_id),
                client_card.max_simultaneous_outputs,
            };

            cards.push_back(display_card);
        }

        /* Outputs */
        for (size_t i = 0; i < client_config.num_outputs; i++)
        {
            auto const& client_output = client_config.outputs[i];
            mg::DisplayConfigurationOutput display_output
            {
                mg::DisplayConfigurationOutputId(client_output.output_id),
                mg::DisplayConfigurationCardId(client_output.card_id),
                static_cast<mg::DisplayConfigurationOutputType>(client_output.type),
                {},
                {},
                client_output.preferred_mode,
                geom::Size{client_output.physical_width_mm,
                           client_output.physical_height_mm},
                static_cast<bool>(client_output.connected),
                static_cast<bool>(client_output.used),
                geom::Point{client_output.position_x,
                            client_output.position_y},
                client_output.current_mode,
                client_output.current_format,
                static_cast<MirPowerMode>(client_output.power_mode),
                static_cast<MirOrientation>(client_output.orientation)
            };

            /* Modes */
            std::vector<mg::DisplayConfigurationMode> modes;
            for (size_t n = 0; n < client_output.num_modes; n++)
            {
                auto const& client_mode = client_output.modes[n];
                modes.push_back(
                    {
                        geom::Size{client_mode.horizontal_resolution,
                                   client_mode.vertical_resolution},
                        client_mode.refresh_rate
                    });
            }
            display_output.modes = modes;

            /* Pixel formats */
            std::vector<MirPixelFormat> pixel_formats;
            for (size_t n = 0; n < client_output.num_output_formats; n++)
            {
                pixel_formats.push_back(client_output.output_formats[n]);
            }
            display_output.pixel_formats = pixel_formats;

            outputs.push_back(display_output);
        }
    }

    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const override
    {
        for (auto const& card : cards)
            f(card);
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const override
    {
        for (auto const& output : outputs)
            f(output);
    }

    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f) override
    {
        for (auto& output : outputs)
        {
            mg::UserDisplayConfigurationOutput user(output);
            f(user);
        }
    }

private:
    std::vector<mg::DisplayConfigurationCard> cards;
    std::vector<mg::DisplayConfigurationOutput> outputs;
};

}

bool mt::compare_display_configurations(mg::DisplayConfiguration const& config1,
                                        mg::DisplayConfiguration const& config2)
{
    bool result = true;

    /* cards */
    std::vector<mg::DisplayConfigurationCard> cards1;
    std::vector<mg::DisplayConfigurationCard> cards2;

    config1.for_each_card(
        [&cards1](mg::DisplayConfigurationCard const& card)
        {
            cards1.push_back(card);
        });
    config2.for_each_card(
        [&cards2](mg::DisplayConfigurationCard const& card)
        {
            cards2.push_back(card);
        });

    if (cards1.size() == cards2.size())
    {
        for (size_t i = 0; i < cards1.size(); i++)
        {
            if (cards1[i] != cards2[i])
            {
                EXPECT_EQ(cards1[i], cards2[i]);
                result = false;
            }
        }
    }
    else
    {
        EXPECT_EQ(cards1.size(), cards2.size());
        result = false;
    }

    /* Outputs */
    std::vector<mg::DisplayConfigurationOutput> outputs1;
    std::vector<mg::DisplayConfigurationOutput> outputs2;

    config1.for_each_output(
        [&outputs1](mg::DisplayConfigurationOutput const& output)
        {
            outputs1.push_back(output);
        });
    config2.for_each_output(
        [&outputs2](mg::DisplayConfigurationOutput const& output)
        {
            outputs2.push_back(output);
        });

    if (outputs1.size() == outputs2.size())
    {
        for (size_t i = 0; i < outputs1.size(); i++)
        {
            if (outputs1[i] != outputs2[i])
            {
                EXPECT_EQ(outputs1[i], outputs2[i]);
                result = false;
            }
        }
    }
    else
    {
        EXPECT_EQ(outputs1.size(), outputs2.size());
        result = false;
    }

    return result;
}

bool mt::compare_display_configurations(MirDisplayConfiguration const& client_config,
                                        mg::DisplayConfiguration const& display_config)
{
    TestDisplayConfiguration config1{client_config};
    return compare_display_configurations(config1, display_config);
}

bool mt::compare_display_configurations(mp::DisplayConfiguration const& protobuf_config,
                                        mg::DisplayConfiguration const& display_config)
{
    TestDisplayConfiguration config1{protobuf_config};
    return compare_display_configurations(config1, display_config);
}

bool mt::compare_display_configurations(MirDisplayConfiguration const* client_config1,
                                        MirDisplayConfiguration const* client_config2)
{
    TestDisplayConfiguration config1{*client_config1};
    TestDisplayConfiguration config2{*client_config2};
    return compare_display_configurations(config1, config2);
}

bool mt::compare_display_configurations(MirDisplayConfiguration const& client_config,
                                        mp::DisplayConfiguration const& protobuf_config)
{
    TestDisplayConfiguration config1{client_config};
    TestDisplayConfiguration config2{protobuf_config};
    return compare_display_configurations(config1, config2);
}

bool mt::compare_display_configurations(graphics::DisplayConfiguration const& display_config1,
                                        MirDisplayConfiguration const* display_config2)
{
    TestDisplayConfiguration config2{*display_config2};
    return compare_display_configurations(display_config1, config2);
}

bool mt::compare_display_configurations(MirDisplayConfiguration const* display_config2,
                                        graphics::DisplayConfiguration const& display_config1)
{
    TestDisplayConfiguration config2{*display_config2};
    return compare_display_configurations(display_config1, config2);
}
