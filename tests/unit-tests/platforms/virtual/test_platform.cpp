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

#include "mir/graphics/platform.h"
#include "mir/options/program_option.h"
#include "src/platforms/virtual/platform.h"

#include "mir/shared_library.h"
#include "mir_test_framework/executable_path.h"
#include "src/server/report/null/display_report.h"

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;
namespace geom = mir::geometry;
using namespace testing;

namespace
{

class VirtualGraphicsPlatformTest : public ::testing::Test
{
public:
    std::shared_ptr<mg::DisplayPlatform> create_platform()
    {
        mgv::VirtualOutputConfig config({{1280, 1024}});
        return std::make_shared<mgv::Platform>(
            std::make_shared<mir::report::null::DisplayReport>(),
            std::vector<mgv::VirtualOutputConfig>{config});
    }
};

TEST_F(VirtualGraphicsPlatformTest, has_one_config_when_provided_one_output_size)
{
    auto configs = mgv::Platform::parse_output_sizes({"1280x1024"});
    EXPECT_THAT(configs.size(), Eq(1));
}

TEST_F(VirtualGraphicsPlatformTest, lone_config_has_one_output_size_when_provided_one_output_size)
{
    auto configs = mgv::Platform::parse_output_sizes({"1280x1024"});
    EXPECT_THAT(configs[0].sizes.size(), Eq(1));
}

TEST_F(VirtualGraphicsPlatformTest, lone_output_size_has_size_provided_by_string)
{
    auto configs = mgv::Platform::parse_output_sizes({"1280x1024"});
    EXPECT_THAT(configs[0].sizes, ElementsAre(geom::Size{1280, 1024}));
}

TEST_F(VirtualGraphicsPlatformTest, multiple_output_sizes_are_read_when_multiple_are_provided)
{
    auto configs = mgv::Platform::parse_output_sizes({"1280x1024:800x600"});
    EXPECT_THAT(configs[0].sizes.size(), Eq(2));
}

TEST_F(VirtualGraphicsPlatformTest, multiple_output_sizes_are_set_correctly_when_multiple_are_provided)
{
    auto configs = mgv::Platform::parse_output_sizes({"1280x1024:800x600"});
    EXPECT_THAT(configs[0].sizes, ElementsAre(
        geom::Size{1280, 1024},
        geom::Size{800, 600}));
}

TEST_F(VirtualGraphicsPlatformTest, multiple_configs_are_parset_when_multiple_strings_are_provided)
{
    auto configs = mgv::Platform::parse_output_sizes({"1280x1024", "800x600"});
    EXPECT_THAT(configs[0].sizes, ElementsAre(geom::Size{1280, 1024}));
    EXPECT_THAT(configs[1].sizes, ElementsAre(geom::Size{800, 600}));
}

}