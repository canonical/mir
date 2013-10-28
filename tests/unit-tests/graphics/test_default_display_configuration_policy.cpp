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

#include "src/server/graphics/default_display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

class MockDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    MockDisplayConfiguration(size_t max_simultaneous_outputs)
        : card_id{1}, max_simultaneous_outputs{max_simultaneous_outputs}
    {
        /* Connected with modes */
        outputs.push_back(
            {
                mg::DisplayConfigurationOutputId{10},
                card_id,
                mg::DisplayConfigurationOutputType::vga,
                {
                    geom::PixelFormat::abgr_8888
                },
                {
                    {geom::Size{123, 111}, 59.9},
                    {geom::Size{123, 111}, 59.9},
                    {geom::Size{123, 111}, 59.9}
                },
                2,
                geom::Size{324, 642},
                true,
                false,
                geom::Point{geom::X{123}, geom::Y{343}},
                1,
                0,
                mir_power_mode_on
            });
        /* Connected without modes */
        outputs.push_back(
            {
                mg::DisplayConfigurationOutputId{11},
                card_id,
                mg::DisplayConfigurationOutputType::vga,
                {},
                {},
                std::numeric_limits<size_t>::max(),
                geom::Size{566, 111},
                true,
                false,
                geom::Point(),
                std::numeric_limits<size_t>::max(),
                std::numeric_limits<size_t>::max(),
                mir_power_mode_on
            });
        /* Connected with a single mode */
        outputs.push_back(
            {
                mg::DisplayConfigurationOutputId{12},
                card_id,
                mg::DisplayConfigurationOutputType::vga,
                {
                    geom::PixelFormat::abgr_8888
                },
                {
                    {geom::Size{523, 555}, 60.0},
                },
                0,
                geom::Size{324, 642},
                true,
                false,
                geom::Point(),
                0,
                0,
                mir_power_mode_on
            });
        /* Not connected */
        outputs.push_back(
            {
                mg::DisplayConfigurationOutputId{13},
                card_id,
                mg::DisplayConfigurationOutputType::vga,
                {
                    geom::PixelFormat::abgr_8888
                },
                {},
                0,
                geom::Size{324, 642},
                false,
                false,
                geom::Point(),
                1,
                0,
                mir_power_mode_on
            });

        if (max_simultaneous_outputs == max_simultaneous_outputs_all)
            max_simultaneous_outputs = outputs.size();
    }

    MockDisplayConfiguration()
        : MockDisplayConfiguration{max_simultaneous_outputs_all}
    {
    }

    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const
    {
        f({card_id, max_simultaneous_outputs});
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
    {
        for (auto const& output : outputs)
            f(output);
    }

    MOCK_METHOD5(configure_output, void(mg::DisplayConfigurationOutputId, bool,
                                        geom::Point, size_t, MirPowerMode));

private:
    static const size_t max_simultaneous_outputs_all{std::numeric_limits<size_t>::max()};
    mg::DisplayConfigurationCardId const card_id;
    size_t max_simultaneous_outputs;
    std::vector<mg::DisplayConfigurationOutput> outputs;
};

}

TEST(DefaultDisplayConfigurationPolicyTest, uses_all_connected_valid_outputs)
{
    using namespace ::testing;

    mg::DefaultDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf;

    conf.for_each_output([&conf](mg::DisplayConfigurationOutput const& output)
    {
        if (output.connected && output.modes.size() > 0)
        {
            EXPECT_CALL(conf, configure_output(output.id, true, geom::Point(),
                                               output.preferred_mode_index, _));
        }
        else
        {
            EXPECT_CALL(conf, configure_output(output.id, false, output.top_left,
                                               output.current_mode_index, _));
        }
    });

    policy.apply_to(conf);
}

TEST(DefaultDisplayConfigurationPolicyTest, default_policy_is_power_mode_on)
{
    using namespace ::testing;

    mg::DefaultDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf;

    conf.for_each_output([&conf](mg::DisplayConfigurationOutput const& output)
    {
        EXPECT_CALL(conf, configure_output(output.id, _, _, _, mir_power_mode_on));
    });

    policy.apply_to(conf);
}

TEST(DefaultDisplayConfigurationPolicyTest, does_not_enable_more_outputs_than_supported)
{
    using namespace ::testing;

    size_t const max_simultaneous_outputs{1};
    mg::DefaultDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf{max_simultaneous_outputs};

    size_t output_count{0};
    conf.for_each_output([&output_count](mg::DisplayConfigurationOutput const&)
    {
        ++output_count;
    });

    EXPECT_CALL(conf, configure_output(_, true, _, _, _))
        .Times(AtMost(max_simultaneous_outputs));

    EXPECT_CALL(conf, configure_output(_, false, _, _, _))
        .Times(AtLeast(output_count - max_simultaneous_outputs));

    policy.apply_to(conf);
}
