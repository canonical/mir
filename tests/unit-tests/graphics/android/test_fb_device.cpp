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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_doubles/mock_fb_hal_device.h"
#include "mir_test_doubles/mock_buffer.h"
#include "src/server/graphics/android/fb_device.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_android_native_buffer.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;
namespace mt=mir::test;

struct FBDevice : public ::testing::Test
{
    virtual void SetUp()
    {
        using namespace testing;
        
        width = 413;
        height = 516;
        fbnum = 4;
        format = HAL_PIXEL_FORMAT_RGBA_8888;

        fb_hal_mock = std::make_shared<mtd::MockFBHalDevice>(width, height, format, fbnum); 
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();

        native_buffer = std::make_shared<mtd::StubAndroidNativeBuffer>(); 
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_buffer));
    }

    unsigned int width, height, format, fbnum;
    std::shared_ptr<mtd::MockFBHalDevice> fb_hal_mock;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<mir::graphics::NativeBuffer> native_buffer;
    mtd::HardwareAccessMock hw_access_mock;
    mtd::MockEGL mock_egl;
};

TEST_F(FBDevice, set_next_frontbuffer_ok)
{
    using namespace testing;
    mga::FBDevice fbdev(fb_hal_mock);

    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(), native_buffer->handle()))
        .Times(1);

    fbdev.set_next_frontbuffer(mock_buffer); 
}

TEST_F(FBDevice, set_next_frontbuffer_fail)
{
    using namespace testing;
    mga::FBDevice fbdev(fb_hal_mock);

    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(),native_buffer->handle()))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        fbdev.set_next_frontbuffer(mock_buffer); 
    }, std::runtime_error); 
}

TEST_F(FBDevice, determine_size)
{
    mga::FBDevice fbdev(fb_hal_mock);
    auto size = fbdev.display_size();
    EXPECT_EQ(width, size.width.as_uint32_t());
    EXPECT_EQ(height, size.height.as_uint32_t());
}

TEST_F(FBDevice, determine_fbnum)
{
    mga::FBDevice fbdev(fb_hal_mock);
    EXPECT_EQ(fbnum, fbdev.number_of_framebuffers_available());
}

TEST_F(FBDevice, commit_frame)
{
    using namespace testing;
    int bad = 0xdfefefe; 
    EGLDisplay dpy = static_cast<EGLDisplay>(&bad);
    EGLSurface surf = static_cast<EGLSurface>(&bad);
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(2)
        .WillOnce(Return(EGL_FALSE))
        .WillOnce(Return(EGL_TRUE));

    mga::FBDevice fbdev(fb_hal_mock);

    EXPECT_THROW({
        fbdev.commit_frame(dpy, surf);
    }, std::runtime_error);
    fbdev.commit_frame(dpy, surf);
}

//some drivers incorrectly report 0 buffers available. if this is true, we should alloc 2, the minimum requirement
TEST_F(FBDevice, determine_fbnum_always_reports_2_minimum)
{
    auto slightly_malformed_fb_hal_mock = std::make_shared<mtd::MockFBHalDevice>(width, height, format, 0); 
    mga::FBDevice fbdev(slightly_malformed_fb_hal_mock);
    EXPECT_EQ(2u, fbdev.number_of_framebuffers_available());
}

TEST_F(FBDevice, determine_pixformat)
{
    mga::FBDevice fbdev(fb_hal_mock);
    EXPECT_EQ(geom::PixelFormat::abgr_8888, fbdev.display_format());
}

TEST_F(FBDevice, set_swapinterval)
{
    EXPECT_CALL(*fb_hal_mock, setSwapInterval_interface(fb_hal_mock.get(), 1))
        .Times(1);
    mga::FBDevice fbdev(fb_hal_mock);

    testing::Mock::VerifyAndClearExpectations(fb_hal_mock.get());

    EXPECT_CALL(*fb_hal_mock, setSwapInterval_interface(fb_hal_mock.get(), 0))
        .Times(1);
    fbdev.sync_to_display(false);
}

TEST_F(FBDevice, set_swapinterval_with_nullhook)
{
    fb_hal_mock->setSwapInterval = nullptr;
    mga::FBDevice fbdev(fb_hal_mock);
    fbdev.sync_to_display(false);
}
