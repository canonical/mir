/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/test/doubles/null_emergency_cleanup.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/stub_console_services.h"
#include "src/platforms/mesa/server/kms/platform.h"
#include "src/platforms/mesa/include/native_buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "src/platforms/mesa/server/buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/display.h"

#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/null_gl_config.h"
#include "mir/test/doubles/null_display_configuration_policy.h"
#include "mir_test_framework/udev_environment.h"

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gbm.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

class MesaBufferAllocatorTest  : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        fake_devices.add_standard_device("standard-drm-devices");

        size = geom::Size{300, 200};
        pf = mir_pixel_format_argb_8888;

        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        ON_CALL(mock_egl, eglGetConfigAttrib(_, mock_egl.fake_configs[0], EGL_NATIVE_VISUAL_ID, _))
            .WillByDefault(
                DoAll(
                    SetArgPointee<3>(GBM_FORMAT_XRGB8888),
                    Return(EGL_TRUE)));

        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

        ON_CALL(mock_gbm, gbm_bo_get_handle(_))
        .WillByDefault(Return(mock_gbm.fake_gbm.bo_handle));

        platform = std::make_shared<mgm::Platform>(
                mir::report::null_display_report(),
                std::make_shared<mtd::StubConsoleServices>(),
                *std::make_shared<mtd::NullEmergencyCleanup>(),
                mgm::BypassOption::allowed);
        display = platform->create_display(
            std::make_shared<mtd::NullDisplayConfigurationPolicy>(),
            std::make_shared<mtd::NullGLConfig>());
        allocator.reset(new mgm::BufferAllocator(
            *display,
            platform->gbm->device,
            mgm::BypassOption::allowed,
            mgm::BufferImportMethod::gbm_native_pixmap));
    }

    // Defaults
    geom::Size size;
    MirPixelFormat pf;

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    std::shared_ptr<mgm::Platform> platform;
    std::unique_ptr<mg::Display> display;
    std::unique_ptr<mgm::BufferAllocator> allocator;
    mtf::UdevEnvironment fake_devices;
};

TEST_F(MesaBufferAllocatorTest, creates_software_buffer_without_utilizing_gbm)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_)).Times(0);
    allocator->alloc_software_buffer( { 1000, 1000}, mir_pixel_format_abgr_8888);
}

TEST_F(MesaBufferAllocatorTest, supported_pixel_formats_contain_common_formats)
{
    auto supported_pixel_formats = allocator->supported_pixel_formats();

    auto argb_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      mir_pixel_format_argb_8888);

    auto xrgb_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      mir_pixel_format_xrgb_8888);

    EXPECT_EQ(1, argb_8888_count);
    EXPECT_EQ(1, xrgb_8888_count);
}

TEST_F(MesaBufferAllocatorTest, supported_pixel_formats_have_sane_default_in_first_position)
{
    auto supported_pixel_formats = allocator->supported_pixel_formats();

    ASSERT_FALSE(supported_pixel_formats.empty());
    EXPECT_EQ(mir_pixel_format_argb_8888, supported_pixel_formats[0]);
}

