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
#include "mir_test_doubles/stub_android_native_buffer.h"
#include "src/platforms/android/fb_device.h"
#include "src/platforms/android/hwc_fallback_gl_renderer.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/mock_swapping_gl_context.h"
#include "mir_test_doubles/stub_renderable_list_compositor.h"
#include "mir_test_doubles/mock_renderable_list_compositor.h"

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
        ON_CALL(mock_context, last_rendered_buffer())
            .WillByDefault(Return(mock_buffer));
    }

    unsigned int width, height, format, fbnum;
    std::shared_ptr<mtd::MockFBHalDevice> fb_hal_mock;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<mir::graphics::NativeBuffer> native_buffer;
    mtd::HardwareAccessMock hw_access_mock;
    testing::NiceMock<mtd::MockSwappingGLContext> mock_context;
};

TEST_F(FBDevice, rejects_overlays)
{
    std::list<std::shared_ptr<mg::Renderable>> renderlist
    {
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>()
    };

    mtd::MockRenderableListCompositor stub_compositor;
    mga::FBDevice fbdev(fb_hal_mock);
    EXPECT_FALSE(fbdev.post_overlays(mock_context, renderlist, stub_compositor));
}

TEST_F(FBDevice, commits_frame_via_post)
{
    using namespace testing;
    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(), native_buffer->handle()))
        .Times(2)
        .WillOnce(Return(-1))
        .WillOnce(Return(0));

    mga::FBDevice fbdev(fb_hal_mock);

    EXPECT_THROW({
        fbdev.post_gl(mock_context);
    }, std::runtime_error);

    fbdev.post_gl(mock_context);
}

TEST_F(FBDevice, sets_swapinterval_1_on_start)
{
    EXPECT_CALL(*fb_hal_mock, setSwapInterval_interface(fb_hal_mock.get(), 1))
        .Times(1);
    mga::FBDevice fbdev(fb_hal_mock);
}

//not all fb devices provide a swap interval hook. make sure we don't explode if thats the case
TEST_F(FBDevice, does_not_segfault_if_null_swapinterval_hook)
{
    fb_hal_mock->setSwapInterval = nullptr;
    mga::FBDevice fbdev(fb_hal_mock);
}

TEST_F(FBDevice, can_screen_on_off)
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
