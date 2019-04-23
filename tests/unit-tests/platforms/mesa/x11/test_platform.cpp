/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/options/program_option.h"
#include "src/platforms/mesa/server/x11/graphics/platform.h"
#include "src/server/report/null/display_report.h"

#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/mock_x11.h"
#include "mir/shared_library.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/udev_environment.h"

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
const char probe_platform[] = "probe_graphics_platform";

class X11GraphicsPlatformTest : public ::testing::Test
{
public:
    void SetUp()
    {
        ::testing::Mock::VerifyAndClearExpectations(&mock_drm);
        ::testing::Mock::VerifyAndClearExpectations(&mock_gbm);
    }

    std::shared_ptr<mg::Platform> create_platform()
    {
        return std::make_shared<mg::X::Platform>(
            std::shared_ptr<::Display>(
                XOpenDisplay(nullptr),
                [](::Display* display)
                {
                    XCloseDisplay(display);
                }),
            std::vector<mir::geometry::Size>{{1280, 1024}},
            std::make_shared<mir::report::null::DisplayReport>());
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<mtd::MockX11> mock_x11;
};
}

TEST_F(X11GraphicsPlatformTest, failure_to_open_x11_display_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_x11, XOpenDisplay(_))
        .WillRepeatedly(Return(nullptr));

    EXPECT_THROW({ create_platform(); }, std::exception);
}

TEST_F(X11GraphicsPlatformTest, failure_to_open_drm_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_drm, open(_,_,_))
        .WillRepeatedly(Return(-1));

    EXPECT_THROW({ create_platform(); }, std::exception);
}

TEST_F(X11GraphicsPlatformTest, failure_to_create_gbm_device_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_gbm, gbm_create_device(_))
        .WillRepeatedly(Return(nullptr));

    EXPECT_THROW({ create_platform(); }, std::exception);
}

TEST_F(X11GraphicsPlatformTest, probe_returns_unsupported_when_no_drm_udev_devices)
{
    mtf::UdevEnvironment udev_environment;
    mir::options::ProgramOption options;

    mir::SharedLibrary platform_lib{mtf::server_platform("server-mesa-x11")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(probe_platform);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(nullptr, options));
}

TEST_F(X11GraphicsPlatformTest, probe_returns_unsupported_when_x_cannot_open_display)
{
    using namespace ::testing;

    mir::options::ProgramOption options;

    EXPECT_CALL(mock_x11, XOpenDisplay(_))
        .WillRepeatedly(Return(nullptr));

    mir::SharedLibrary platform_lib{mtf::server_platform("server-mesa-x11")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(probe_platform);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(nullptr, options));
}

TEST_F(X11GraphicsPlatformTest, probe_returns_supported_when_drm_render_nodes_exist)
{
    mtf::UdevEnvironment udev_environment;
    mir::options::ProgramOption options;

    udev_environment.add_standard_device("standard-drm-render-nodes");

    mir::SharedLibrary platform_lib{mtf::server_platform("server-mesa-x11")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(probe_platform);
    EXPECT_EQ(mg::PlatformPriority::supported, probe(nullptr, options));
}

TEST_F(X11GraphicsPlatformTest, parses_simple_output_size)
{
    using namespace ::testing;

    auto str = "1280x720";
    auto parsed = mg::X::Platform::parse_output_sizes(str);

    EXPECT_THAT(parsed, Eq(std::vector<mir::geometry::Size>{{1280, 720}}));
}

TEST_F(X11GraphicsPlatformTest, parses_multiple_output_size)
{
    using namespace ::testing;

    auto str = "1280x1024,600x600,30x750";
    auto parsed = mg::X::Platform::parse_output_sizes(str);

    EXPECT_THAT(parsed, Eq(std::vector<mir::geometry::Size>{{1280, 1024}, {600, 600}, {30, 750}}));
}

TEST_F(X11GraphicsPlatformTest, output_size_parsing_throws_on_bad_input)
{
    using namespace ::testing;
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("x1280"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x720,"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x720,500"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x1024:1280x1024"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("0x0"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("0x200"), std::runtime_error);
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("50x50,20,50x50"), std::runtime_error);
}
