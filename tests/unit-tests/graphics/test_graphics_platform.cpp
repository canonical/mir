/*
 * Copyright © 2012 Canonical Ltd.
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
#include "mir/test/doubles/platform_factory.h"
#if defined(MESA_KMS) || defined(MESA_X11)
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir_test_framework/udev_environment.h"
#elif ANDROID
#include "mir/test/doubles/mock_android_hw.h"
#endif
#ifdef MESA_X11
#include "mir/test/doubles/mock_x11.h"
#endif

#include "mir/logging/dumb_console_logger.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mo = mir::options;
#if defined(MESA_KMS) || defined(MESA_X11)
namespace mtf = mir_test_framework;
#endif

class GraphicsPlatform : public ::testing::Test
{
public:
    GraphicsPlatform() : logger(std::make_shared<ml::DumbConsoleLogger>())
    {
        using namespace testing;

#if defined(MESA_KMS) || defined(MESA_X11)
        ON_CALL(mock_gbm, gbm_bo_get_width(_))
        .WillByDefault(Return(320));

        ON_CALL(mock_gbm, gbm_bo_get_height(_))
        .WillByDefault(Return(240));

        // FIXME: This format needs to match Mesa's first supported pixel
        //        format or tests will fail. The coupling is presently loose.
        ON_CALL(mock_gbm, gbm_bo_get_format(_))
        .WillByDefault(Return(GBM_FORMAT_ARGB8888));

#ifdef MESA_KMS
        fake_devices.add_standard_device("standard-drm-devices");
#else
        fake_devices.add_standard_device("standard-drm-render-nodes");
#endif
#endif
    }

    std::shared_ptr<mg::Platform> create_platform()
    {
        return mtd::create_platform_with_null_dependencies();
    }

    std::shared_ptr<ml::Logger> logger;

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
#ifdef ANDROID
    ::testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
#else
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    mtf::UdevEnvironment fake_devices;
#endif
#ifdef MESA_X11
    ::testing::NiceMock<mtd::MockX11> mock_x11;
#endif
};

TEST_F(GraphicsPlatform, buffer_allocator_creation)
{
    using namespace testing;

    EXPECT_NO_THROW (
        auto platform = create_platform();
        auto allocator = platform->create_buffer_allocator();

        EXPECT_TRUE(allocator.get());
    );
}

TEST_F(GraphicsPlatform, buffer_creation)
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

TEST_F(GraphicsPlatform, connection_ipc_package)
{
    auto platform = create_platform();
    auto ipc_ops = platform->make_ipc_operations();
    auto pkg = ipc_ops->connection_ipc_package();

    ASSERT_TRUE(pkg.get() != NULL);
}
