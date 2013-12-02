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
#include "src/platform/graphics/android/fb_device.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_android_native_buffer.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
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

        fb_hal_mock = std::make_shared<NiceMock<mtd::MockFBHalDevice>>(width, height, format, fbnum);
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

TEST_F(FBDevice, render)
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
        fbdev.gpu_render(dpy, surf);
    }, std::runtime_error);

    fbdev.gpu_render(dpy, surf);
}

TEST_F(FBDevice, commit_frame)
{
    using namespace testing;
    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(), native_buffer->handle()))
        .Times(2)
        .WillOnce(Return(-1))
        .WillOnce(Return(0));

    mga::FBDevice fbdev(fb_hal_mock);

    EXPECT_THROW({
        fbdev.post(*mock_buffer);
    }, std::runtime_error);

    fbdev.post(*mock_buffer);
}

TEST_F(FBDevice, set_swapinterval)
{
    EXPECT_CALL(*fb_hal_mock, setSwapInterval_interface(fb_hal_mock.get(), 1))
        .Times(1);
    mga::FBDevice fbdev(fb_hal_mock);
}

//not all fb devices provide a swap interval hook. make sure we don't explode if thats the case
TEST_F(FBDevice, set_swapinterval_with_null_hook)
{
    fb_hal_mock->setSwapInterval = nullptr;
    mga::FBDevice fbdev(fb_hal_mock);
}

TEST_F(FBDevice, screen_on_off)
{
    fb_hal_mock->setSwapInterval = nullptr;
    using namespace testing;
    //constructor turns on
    Sequence seq;
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,1))
        .InSequence(seq);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,0))
        .InSequence(seq);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,0))
        .InSequence(seq);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,0))
        .InSequence(seq);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,1))
        .InSequence(seq);
 
    mga::FBDevice fbdev(fb_hal_mock);
    fbdev.mode(mir_power_mode_standby);
    fbdev.mode(mir_power_mode_suspend);
    fbdev.mode(mir_power_mode_off);
    fbdev.mode(mir_power_mode_on);
}
