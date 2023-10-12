/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "src/platforms/virtual/display.h"
#include "src/platforms/virtual/platform.h"

#include "mir/graphics/display_configuration.h"

#include "mir/test/doubles/null_display_configuration_policy.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/fake_shared.h"


namespace mg = mir::graphics;
namespace mgv = mg::virt;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;
using namespace testing;


namespace
{


class VirtualDisplayTest : public ::testing::Test
{
public:
    VirtualDisplayTest()
    {
    }

    std::shared_ptr<mgv::Display> create_display(std::vector<mgv::VirtualOutputConfig> sizes)
    {
        return std::make_shared<mgv::Display>(sizes);
    }

    mtd::NullDisplayConfigurationPolicy null_display_configuration_policy;
    ::testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(VirtualDisplayTest, reports_a_mode_size_that_matches_the_provided_size)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({geom::Size{1280, 1024}})
    });
    auto config = display->configuration();
    int output_count = 0;
    config->for_each_output([&output_count](mg::DisplayConfigurationOutput const& output)
    {
        EXPECT_THAT(output.modes.size(), Eq(1));
        EXPECT_THAT(output.modes[0].size, Eq(geom::Size{1280, 1024}));
        output_count++;
    });

    EXPECT_THAT(output_count, Eq(1));
}

TEST_F(VirtualDisplayTest, reports_multiple_mode_sizes_that_matches_the_provided_sizes)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({geom::Size{1280, 1024}, geom::Size{800, 600}})
    });
    auto config = display->configuration();
    int output_count = 0;
    config->for_each_output([&output_count](mg::DisplayConfigurationOutput const& output)
    {
        EXPECT_THAT(output.modes.size(), Eq(2));
        EXPECT_THAT(output.modes[0].size, Eq(geom::Size{1280, 1024}));
        EXPECT_THAT(output.modes[1].size, Eq(geom::Size{800, 600}));
        output_count++;
    });

    EXPECT_THAT(output_count, Eq(1));
}

TEST_F(VirtualDisplayTest, reports_multiple_outputs_when_provided_multiple_outputs)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({geom::Size{1280, 1024}}),
        mgv::VirtualOutputConfig({geom::Size{1280, 1024}})
    });
    auto config = display->configuration();
    int output_count = 0;
    config->for_each_output([&output_count](mg::DisplayConfigurationOutput const&)
    {
        output_count++;
    });

    EXPECT_THAT(output_count, Eq(2));
}

TEST_F(VirtualDisplayTest, for_each_display_group_iterates_no_display_groups)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({geom::Size{1280, 1024}}),
        mgv::VirtualOutputConfig({geom::Size{1280, 1024}})
    });

    int count = 0;
    display->for_each_display_sync_group([&count](auto&)
    {
        count++;
    });

    EXPECT_THAT(count, Eq(0));
}

}