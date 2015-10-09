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

#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/display_configuration.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

#include <gtest/gtest.h>

#include <vector>
#include <algorithm>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

class StubDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    struct OutputInfo
    {
        OutputInfo(
            geom::Rectangle const& rect,
            bool connected,
            bool used,
            MirOrientation orientation,
            MirPowerMode power_mode = mir_power_mode_on)
            : rect(rect), connected{connected}, used{used},
              orientation{orientation}, power_mode{power_mode}
        {
        }

        geom::Rectangle rect;
        bool connected;
        bool used;
        MirOrientation orientation;
        MirPowerMode power_mode;
    };

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
        uint32_t i = 1;
        for (auto const& info : outputs)
        {
            std::vector<mg::DisplayConfigurationMode> modes(i);
            modes[i - 1] = {info.rect.size, 59.9};

            mg::DisplayConfigurationOutput output
            {
                mg::DisplayConfigurationOutputId(i),
                mg::DisplayConfigurationCardId{1},
                mg::DisplayConfigurationOutputType::svideo,
                {},
                modes,
                i - 1,
                {100, 100},
                info.connected,
                info.used,
                info.rect.top_left,
                i - 1,
                mir_pixel_format_invalid,
                info.power_mode,
                info.orientation,
                1.0f,
                mir_form_factor_monitor
            };

            f(output);
            i++;
        }
    }

    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)>) override
    {
    }

    std::vector<OutputInfo> outputs;
};

class OverlappingOutputGroupingTest : public testing::Test
{
public:
    void check_groupings(std::vector<StubDisplayConfiguration::OutputInfo> const& info,
                         std::vector<geom::Rectangles> const& expected_groups)
    {
        StubDisplayConfiguration conf{info};
        mg::OverlappingOutputGrouping grouping{conf};

        std::vector<std::vector<mg::DisplayConfigurationOutput>> grouping_results;

        grouping.for_each_group(
            [&](mg::OverlappingOutputGroup const& group)
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
                rects.add(output.extents());

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
        {{{0,0}, {100, 100}}, false, false, mir_orientation_normal},
        {{{0,0}, {100, 100}}, true, false, mir_orientation_normal},
        {{{0,0}, {100, 100}}, false, true, mir_orientation_normal}
    };

    std::vector<geom::Rectangles> expected_groups;

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, distinct_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100, 100}}, true, true, mir_orientation_normal},
        {{{100,0}, {100, 100}}, true, true, mir_orientation_normal},
        {{{0,100}, {100, 100}}, true, true, mir_orientation_normal}
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles{{{0,0}, {100, 100}}},
        geom::Rectangles{{{100,0}, {100, 100}}},
        geom::Rectangles{{{0,100}, {100, 100}}},
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, rotated_output)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100,200}}, true, true, mir_orientation_left},
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles{{{0,0}, {200,100}}},
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, rotated_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100,200}}, true, true, mir_orientation_normal},
        {{{1000,0}, {300,400}}, true, true, mir_orientation_left},
        {{{2000,0}, {500,600}}, true, true, mir_orientation_right},
        {{{3000,0}, {700,800}}, true, true, mir_orientation_inverted},
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles{{{0,0}, {100,200}}},
        geom::Rectangles{{{1000,0}, {400,300}}},
        geom::Rectangles{{{2000,0}, {600,500}}},
        geom::Rectangles{{{3000,0}, {700,800}}},
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, rotation_creates_overlap)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100,200}}, true, true, mir_orientation_left},
        {{{100,0}, {100,200}}, true, true, mir_orientation_left},
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles{ {{0,0}, {200,100}},
                          {{100,0}, {200,100}} }
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, different_orientation_prevents_grouping)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100,200}}, true, true, mir_orientation_left},
        {{{100,0}, {100,200}}, true, true, mir_orientation_normal},
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles{ {{0,0}, {200,100}} },
        geom::Rectangles{ {{100,0}, {100,200}} },
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, overlapping_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100, 100}}, true, true, mir_orientation_normal},
        {{{0,0}, {50, 50}}, true, true, mir_orientation_normal},
        {{{100,0}, {100, 100}}, true, true, mir_orientation_normal},
        {{{101,0}, {100, 100}}, true, true, mir_orientation_normal}
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles
        {
            {{0,0}, {100, 100}},
            {{0,0}, {50, 50}}
        },
        geom::Rectangles
        {
            {{100,0}, {100, 100}},
            {{101,0}, {100, 100}}
        }
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, multiply_overlapping_outputs)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100, 100}}, true, true, mir_orientation_normal},
        {{{150,150}, {50, 50}}, true, true, mir_orientation_normal},
        {{{90,90}, {85, 85}}, true, true, mir_orientation_normal},
        {{{50,50}, {100, 100}}, true, true, mir_orientation_normal}
    };

    std::vector<geom::Rectangles> expected_groups
    {
        geom::Rectangles
        {
            {{0,0}, {100, 100}},
            {{150,150}, {50, 50}},
            {{90,90}, {85, 85}},
            {{50,50}, {100, 100}}
        },
    };

    check_groupings(info, expected_groups);
}

TEST_F(OverlappingOutputGroupingTest, ignores_outputs_with_power_mode_not_on)
{
    std::vector<StubDisplayConfiguration::OutputInfo> info
    {
        {{{0,0}, {100, 100}}, true, true, mir_orientation_normal, mir_power_mode_off},
        {{{0,0}, {100, 100}}, true, true, mir_orientation_normal, mir_power_mode_standby},
        {{{0,0}, {100, 100}}, true, true, mir_orientation_normal, mir_power_mode_suspend}
    };

    std::vector<geom::Rectangles> expected_groups;

    check_groupings(info, expected_groups);
}
