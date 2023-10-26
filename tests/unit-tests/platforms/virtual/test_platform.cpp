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
#include "src/platforms/virtual/platform.h"
#include "mir/test/doubles/mock_egl.h"

#include "mir/shared_library.h"
#include "src/server/report/null/display_report.h"

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
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

protected:
    virtual void SetUp()
    {
        mock_egl.provide_egl_extensions();
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    EGLDisplay fake_display{reinterpret_cast<EGLDisplay>(0xabcd)};
};

TEST_F(VirtualGraphicsPlatformTest, lone_config_has_one_output_size_when_provided_one_output_size)
{
    auto configs = mgv::Platform::parse_output_sizes({"1280x1024"});
    EXPECT_THAT(configs, ElementsAre(
        mgv::VirtualOutputConfig(std::vector<geom::Size>{{1280, 1024}})));
}

TEST_F(VirtualGraphicsPlatformTest, multiple_output_sizes_are_set_correctly_when_multiple_are_provided)
{
    auto config = mgv::Platform::parse_output_sizes({"1280x1024:800x600"});
    EXPECT_THAT(config, ElementsAre(
        mgv::VirtualOutputConfig(std::vector<geom::Size>{geom::Size{1280, 1024}, geom::Size{800, 600}})));
}

TEST_F(VirtualGraphicsPlatformTest, can_acquire_interface_for_cpu_addressable_display_provider)
{
    auto platform = create_platform();
    auto interface = mg::DisplayPlatform::interface_for(platform);
    EXPECT_TRUE(interface->acquire_interface<mg::CPUAddressableDisplayProvider>() != nullptr);
}

TEST_F(VirtualGraphicsPlatformTest, cannot_acquire_interface_for_generic_egl_display_provider_when_egl_display_is_none)
{
    ON_CALL(mock_egl, eglGetDisplay(_))
        .WillByDefault(Return(EGL_NO_DISPLAY));
    ON_CALL(mock_egl, eglInitialize(_,_,_))
        .WillByDefault(Return(EGL_TRUE));
    auto platform = create_platform();
    auto interface = mg::DisplayPlatform::interface_for(platform);
    EXPECT_THAT(interface->acquire_interface<mg::GenericEGLDisplayProvider>(), nullptr);
}

TEST_F(VirtualGraphicsPlatformTest, output_size_parsing_throws_on_bad_input)
{
    // X11's tests check the remaining cases here. These are the only cases that do not overlap with X11's.
    using namespace ::testing;
    EXPECT_THROW(mgv::Platform::parse_output_sizes({"1280"}), std::runtime_error) << "No height or 'x'";
    EXPECT_THROW(mgv::Platform::parse_output_sizes({"1280x"}), std::runtime_error) << "No height";
    EXPECT_THROW(mgv::Platform::parse_output_sizes({"x1280"}), std::runtime_error) << "No width";
}

}