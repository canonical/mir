/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/null_virtual_terminal.h"
#include "mir/options/program_option.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir_test_framework/udev_environment.h"
#include "src/platforms/mesa/server/kms/platform.h"

#include "mir/logging/dumb_console_logger.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace ml = mir::logging;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mo = mir::options;
namespace mtf = mir_test_framework;

class GraphicsPlatformKMS : public ::testing::Test
{
public:
    GraphicsPlatformKMS() : logger(std::make_shared<ml::DumbConsoleLogger>())
    {
        using namespace testing;

        ON_CALL(mock_gbm, gbm_bo_get_width(_))
        .WillByDefault(Return(320));

        ON_CALL(mock_gbm, gbm_bo_get_height(_))
        .WillByDefault(Return(240));

        // FIXME: This format needs to match Mesa's first supported pixel
        //        format or tests will fail. The coupling is presently loose.
        ON_CALL(mock_gbm, gbm_bo_get_format(_))
        .WillByDefault(Return(GBM_FORMAT_ARGB8888));

        fake_devices.add_standard_device("standard-drm-devices");
    }

    std::shared_ptr<mg::Platform> create_platform()
    {
      return std::make_shared<mgm::Platform>(
              mir::report::null_display_report(),
              std::make_shared<mtd::NullVirtualTerminal>(),
              *std::make_shared<mtd::NullEmergencyCleanup>(),
              mgm::BypassOption::allowed);
    }

    std::shared_ptr<ml::Logger> logger;

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    mtf::UdevEnvironment fake_devices;
};

TEST_F(GraphicsPlatformKMS, buffer_allocator_creation)
{
    using namespace testing;

    EXPECT_NO_THROW (
        auto platform = create_platform();
        auto allocator = platform->create_buffer_allocator();

        EXPECT_TRUE(allocator.get());
    );
}

TEST_F(GraphicsPlatformKMS, buffer_creation)
{
    auto platform = create_platform();
    auto allocator = platform->create_buffer_allocator();
    auto supported_pixel_formats = allocator->supported_pixel_formats();

    ASSERT_NE(0u, supported_pixel_formats.size());

    geom::Size size{320, 240};
    MirPixelFormat const pf{supported_pixel_formats[0]};
    mg::BufferUsage usage{mg::BufferUsage::hardware};
    mg::BufferProperties buffer_properties{size, pf, usage};

    auto buffer = allocator->alloc_buffer(buffer_properties);

    ASSERT_TRUE(buffer.get() != NULL);

    EXPECT_EQ(buffer->size(), size);
    EXPECT_EQ(buffer->pixel_format(), pf);
}

TEST_F(GraphicsPlatformKMS, connection_ipc_package)
{
    auto platform = create_platform();
    auto ipc_ops = platform->make_ipc_operations();
    auto pkg = ipc_ops->connection_ipc_package();

    ASSERT_TRUE(pkg.get() != NULL);
}
