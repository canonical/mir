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
#include "mir/graphics/default_display_configuration_policy.h"

#include "mir/test/doubles/null_display_configuration_policy.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/fake_shared.h"


namespace mg = mir::graphics;
namespace mgv = mg::virt;
namespace mt = mir::test;
namespace mtd = mt::doubles;
using namespace testing;
using namespace mir::geometry;

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
        mgv::VirtualOutputConfig({Size{1280, 1024}})
    });
    auto config = display->configuration();
    int output_count = 0;
    config->for_each_output([&output_count](mg::DisplayConfigurationOutput const& output)
    {
        EXPECT_THAT(output.modes.size(), Eq(1));
        EXPECT_THAT(output.modes[0].size, Eq(Size{1280, 1024}));
        output_count++;
    });

    EXPECT_THAT(output_count, Eq(1));
}

TEST_F(VirtualDisplayTest, reports_multiple_mode_sizes_that_matches_the_provided_sizes)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({Size{1280, 1024}, Size{800, 600}})
    });
    auto config = display->configuration();
    int output_count = 0;
    config->for_each_output([&output_count](mg::DisplayConfigurationOutput const& output)
    {
        EXPECT_THAT(output.modes.size(), Eq(2));
        EXPECT_THAT(output.modes[0].size, Eq(Size{1280, 1024}));
        EXPECT_THAT(output.modes[1].size, Eq(Size{800, 600}));
        output_count++;
    });

    EXPECT_THAT(output_count, Eq(1));
}

TEST_F(VirtualDisplayTest, reports_multiple_outputs_when_provided_multiple_outputs)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({Size{1280, 1024}}),
        mgv::VirtualOutputConfig({Size{1280, 1024}})
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
        mgv::VirtualOutputConfig({Size{1280, 1024}}),
        mgv::VirtualOutputConfig({Size{1280, 1024}})
    });

    int count = 0;
    display->for_each_display_sync_group([&count](auto&)
    {
        count++;
    });

    EXPECT_THAT(count, Eq(0));
}

}

TEST_F(VirtualDisplayTest, displays_can_be_positioned)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({Size{1280, 1024}}),
        mgv::VirtualOutputConfig({Size{1280, 1024}})
    });

    {
        auto const conf = display->configuration();

        mg::SideBySideDisplayConfigurationPolicy{}.apply_to(*conf);

        EXPECT_THAT(display->apply_if_configuration_preserves_display_buffers(*conf), IsTrue());
    }

    auto const conf = display->configuration();

    std::vector<Point> positions;

    conf->for_each_output([&](mg::DisplayConfigurationOutput const& c) { positions.push_back(c.top_left); });

    EXPECT_THAT(positions, ElementsAre(Point{0, 0}, Point{1280, 0}));
}

TEST_F(VirtualDisplayTest, displays_can_be_inverted)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({Size{1280, 1024}}),
        mgv::VirtualOutputConfig({Size{1280, 1024}})
    });

    {
        auto const conf = display->configuration();

        conf->for_each_output([](mg::UserDisplayConfigurationOutput& conf) { conf.orientation = mir_orientation_inverted; });

        EXPECT_THAT(display->apply_if_configuration_preserves_display_buffers(*conf), IsTrue());
    }

    auto const conf = display->configuration();

    std::vector<MirOrientation> orientations;

    conf->for_each_output([&](mg::DisplayConfigurationOutput const& c) { orientations.push_back(c.orientation); });

    EXPECT_THAT(orientations, ElementsAre(mir_orientation_inverted, mir_orientation_inverted));
}

TEST_F(VirtualDisplayTest, displays_can_be_resized_without_reallocating_buffers)
{
    auto display = create_display({
        mgv::VirtualOutputConfig({Size{1280, 1024}, Size{640, 512}}),
        mgv::VirtualOutputConfig({Size{1280, 1024}})
    });

    {
        auto const conf = display->configuration();

        conf->for_each_output([](mg::UserDisplayConfigurationOutput& conf)
            {
                if (conf.modes.size() > 1)
                {
                    conf.current_mode_index = 1;
                }
            });

        // This may seem unexpected, but there are no buffers for this platform
        EXPECT_THAT(display->apply_if_configuration_preserves_display_buffers(*conf), IsTrue());
        display->configure(*conf);
    }

    auto const conf = display->configuration();

    std::vector<Size> sizes;

    conf->for_each_output([&](mg::DisplayConfigurationOutput const& c)
        { sizes.push_back(c.modes[c.current_mode_index].size); });

    EXPECT_THAT(sizes, ElementsAre(Size{640, 512}, Size{1280, 1024}));
}
