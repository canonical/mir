/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/graphics/gbm/overlapping_output_grouping.h"
#include "mir/graphics/display_configuration.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

#include <gtest/gtest.h>

#include <vector>
#include <algorithm>
#include <tuple>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;

namespace
{

class StubDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    typedef std::tuple<geom::Rectangle,bool,bool> OutputInfo;

    StubDisplayConfiguration(std::vector<OutputInfo> const& info)
        : outputs{info}
    {
    }

    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const
    {
        mg::DisplayConfigurationCard card
        {
            mg::DisplayConfigurationCardId{1},
            outputs.size()
        };

        f(card);
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
    {
        size_t i = 1;
        for (auto const& info : outputs)
        {
            auto const& rect = std::get<0>(info);
            auto const connected = std::get<1>(info);
            auto const used = std::get<2>(info);
            std::vector<mg::DisplayConfigurationMode> modes(i);
            modes[i - 1] = {rect.size, 59.9};

            mg::DisplayConfigurationOutput output
            {
                mg::DisplayConfigurationOutputId(i),
                mg::DisplayConfigurationCardId{1},
                mg::DisplayConfigurationOutputType::svideo,
                {},
                modes,
                i - 1,
                {100, 100},
                connected,
                used,
                rect.top_left,
                i - 1,
                0,
                mir_power_mode_on
            };

            f(output);
            i++;
        }
    }

    void configure_output(mg::DisplayConfigurationOutputId, bool,
                          geom::Point, size_t, MirPowerMode)
    {
    }

    std::vector<std::tuple<geom::Rectangle,bool,bool>> outputs;
};

class OverlappingOutputGroupingTest : public testing::Test
{
public:
    void check_groupings(std::vector<StubDisplayConfiguration::OutputInfo> const& info,
                         std::vector<geom::Rectangles> const& expected_groups)
    {
        StubDisplayConfiguration conf{info};
        mgg::OverlappingOutputGrouping grouping{conf};

        std::vector<std::vector<mg::DisplayConfigurationOutput>> grouping_results;

        grouping.for_each_group(
            [&](mgg::OverlappingOutputGroup const& group)
            {
                std::vector<mg::DisplayConfigurationOutput> outputs;
                group.for_each_output([&](mg::DisplayConfigurationOutput const& output)
                {
                    outputs.push_back(output);
                });
                grouping_results.push_back(outputs);
            });

        auto expected_groups_copy = expected_groups;

        EXPECT_EQ(expected_groups.size(), grouping_results.size());

        for (auto const& v : grouping_results)
        {
            geom::Rectangles rects;
            for (auto const& output : v)
                rects.add({output.top_left, output.modes[output.current_mode_index].size});

            auto iter = std::find(expected_groups_copy.begin(), expected_groups_copy.end(), rects);
            EXPECT_TRUE(iter != expected_groups_copy.end()) << "Failed to find " << rects;
            expected_groups_copy.erase(iter);
        }
    }
};

}

TEST_F(OverlappingOutputGroupingTest, ignores_invalid_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        std::make_tuple(geom::Rectangle{{0,0}, {100, 100}}, false, false),
        std::make_tuple(geom::Rectangle{{0,0}, {100, 100}}, true, false),
        std::make_tuple(geom::Rectangle{{0,0}, {100, 100}}, false, true)
    };

    std::vector<geom::Rectangles> expected_groups;

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, distinct_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        std::make_tuple(geom::Rectangle{{0,0}, {100, 100}}, true, true),
        std::make_tuple(geom::Rectangle{{100,0}, {100, 100}}, true, true),
        std::make_tuple(geom::Rectangle{{0,100}, {100, 100}}, true, true)
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles{geom::Rectangle{{0,0}, {100, 100}}},
        geom::Rectangles{geom::Rectangle{{100,0}, {100, 100}}},
        geom::Rectangles{geom::Rectangle{{0,100}, {100, 100}}},
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, overlapping_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        std::make_tuple(geom::Rectangle{{0,0}, {100, 100}}, true, true),
        std::make_tuple(geom::Rectangle{{0,0}, {50, 50}}, true, true),
        std::make_tuple(geom::Rectangle{{100,0}, {100, 100}}, true, true),
        std::make_tuple(geom::Rectangle{{101,0}, {100, 100}}, true, true)
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles
        {
            geom::Rectangle{{0,0}, {100, 100}},
            geom::Rectangle{{0,0}, {50, 50}}
        },
        geom::Rectangles
        {
            geom::Rectangle{{100,0}, {100, 100}},
            geom::Rectangle{{101,0}, {100, 100}}
        }
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, multiply_overlapping_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        std::make_tuple(geom::Rectangle{{0,0}, {100, 100}}, true, true),
        std::make_tuple(geom::Rectangle{{150,150}, {50, 50}}, true, true),
        std::make_tuple(geom::Rectangle{{90,90}, {85, 85}}, true, true),
        std::make_tuple(geom::Rectangle{{50,50}, {100, 100}}, true, true)
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles
        {
            geom::Rectangle{{0,0}, {100, 100}},
            geom::Rectangle{{150,150}, {50, 50}},
            geom::Rectangle{{90,90}, {85, 85}},
            geom::Rectangle{{50,50}, {100, 100}}
        },
    };

    check_groupings(info, expected_groups);
}
