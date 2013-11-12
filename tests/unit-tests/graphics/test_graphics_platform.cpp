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
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#ifndef ANDROID
#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"
#include "mir_test_doubles/null_virtual_terminal.h"
#include "src/server/graphics/gbm/gbm_platform.h"
#include "mir_test_framework/udev_environment.h"
#else
#include "mir_test_doubles/mock_android_hw.h"
#endif
#include "mir/graphics/buffer_initializer.h"
#include "mir/logging/dumb_console_logger.h"
#include "mir/options/program_option.h"

#include "mir/graphics/null_display_report.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mo = mir::options;
#ifndef ANDROID
namespace mtf = mir::mir_test_framework;
#endif

class GraphicsPlatform : public ::testing::Test
{
public:
    GraphicsPlatform() : logger(std::make_shared<ml::DumbConsoleLogger>())
    {
        using namespace testing;
        buffer_initializer = std::make_shared<mg::NullBufferInitializer>();

#ifndef ANDROID
        ON_CALL(mock_gbm, gbm_bo_get_width(_))
        .WillByDefault(Return(320));

        ON_CALL(mock_gbm, gbm_bo_get_height(_))
        .WillByDefault(Return(240));

        ON_CALL(mock_gbm, gbm_bo_get_format(_))
        .WillByDefault(Return(GBM_FORMAT_ARGB8888));

        typedef mtd::MockEGL::generic_function_pointer_t func_ptr_t;

        ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglCreateImageKHR")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(eglCreateImageKHR)));
        ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglDestroyImageKHR")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(eglDestroyImageKHR)));
        ON_CALL(mock_egl, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(glEGLImageTargetTexture2DOES)));

        fake_devices.add_standard_drm_devices();
#endif
    }

    std::shared_ptr<mg::Platform> create_platform()
    {
#ifdef ANDROID
        return mg::create_platform(std::make_shared<mo::ProgramOption>(), std::make_shared<mg::NullDisplayReport>());
#else
        return std::make_shared<mg::gbm::GBMPlatform>(
            std::make_shared<mg::NullDisplayReport>(),
            std::make_shared<mir::test::doubles::NullVirtualTerminal>());
#endif
    }

    std::shared_ptr<ml::Logger> logger;
    std::shared_ptr<mg::BufferInitializer> buffer_initializer;

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
#ifdef ANDROID
    ::testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
#else
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    mtf::UdevEnvironment fake_devices;
#endif
};

TEST_F(GraphicsPlatform, buffer_allocator_creation)
{
    using namespace testing;

    EXPECT_NO_THROW (
        auto platform = create_platform();
        auto allocator = platform->create_buffer_allocator(buffer_initializer);

        EXPECT_TRUE(allocator.get());
    );

}

TEST_F(GraphicsPlatform, buffer_creation)
{
    auto platform = create_platform();
    auto allocator = platform->create_buffer_allocator(buffer_initializer);
    auto supported_pixel_formats = allocator->supported_pixel_formats();

    ASSERT_NE(0u, supported_pixel_formats.size());

    geom::Size size{320, 240};
    geom::PixelFormat const pf{supported_pixel_formats[0]};
    mg::BufferUsage usage{mg::BufferUsage::hardware};
    mg::BufferProperties buffer_properties{size, pf, usage};

    auto buffer = allocator->alloc_buffer(buffer_properties);

    ASSERT_TRUE(buffer.get() != NULL);

    EXPECT_EQ(buffer->size(), size);
    EXPECT_EQ(buffer->pixel_format(), pf );

}

TEST_F(GraphicsPlatform, get_ipc_package)
{
    auto platform = create_platform();
    auto pkg = platform->get_ipc_package();

    ASSERT_TRUE(pkg.get() != NULL);
}
