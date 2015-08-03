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

#include "src/platforms/android/server/hwc_fb_device.h"
#include "src/platforms/android/server/hwc_configuration.h"
#include "mir/test/doubles/stub_android_native_buffer.h"
#include "mir/test/doubles/mock_display_device.h"
#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/mock_framebuffer_bundle.h"
#include "mir/test/doubles/mock_fb_hal_device.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/stub_swapping_gl_context.h"
#include "mir/test/doubles/mock_swapping_gl_context.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/doubles/mock_hwc_device_wrapper.h"
#include "mir/test/doubles/stub_renderable_list_compositor.h"
#include "src/platforms/android/server/hwc_fallback_gl_renderer.h"
#include "hwc_struct_helpers.h"
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <atomic>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{
class HwcFbDevice : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        int width = 88;
        int height = 4;
        test_size = geom::Size{width, height};
        int fbnum = 558;
        mock_fb_device = std::make_shared<mtd::MockFBHalDevice>(
            width, height, HAL_PIXEL_FORMAT_RGBA_8888, fbnum);
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();
        mock_hwc_device_wrapper = std::make_shared<testing::NiceMock<mtd::MockHWCDeviceWrapper>>();

        stub_native_buffer = std::make_shared<mtd::StubAndroidNativeBuffer>(test_size);
        hwc_rect_t region = {0, 0, width, height};
        skip_layer.compositionType = HWC_FRAMEBUFFER;
        skip_layer.hints = 0;
        skip_layer.flags = HWC_SKIP_LAYER;
        skip_layer.handle = &stub_native_buffer->native_handle;
        skip_layer.transform = 0;
        skip_layer.blending = HWC_BLENDING_NONE;
        skip_layer.sourceCrop = region;
        skip_layer.displayFrame = region;
        skip_layer.visibleRegionScreen = {1, &region};
        skip_layer.acquireFenceFd = -1;
        skip_layer.releaseFenceFd = -1;
        skip_layer.planeAlpha = std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max();

        ON_CALL(*mock_buffer, size())
            .WillByDefault(Return(test_size));
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(stub_native_buffer));
        ON_CALL(mock_context, last_rendered_buffer())
            .WillByDefault(Return(mock_buffer));
    }

    int fake_dpy = 0;
    int fake_sur = 0;
    EGLDisplay dpy{&fake_dpy};
    EGLSurface sur{&fake_sur};

    testing::NiceMock<mtd::MockEGL> mock_egl;

    geom::Size test_size;
    std::shared_ptr<mtd::MockFBHalDevice> mock_fb_device;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<mtd::MockHWCDeviceWrapper> mock_hwc_device_wrapper;
    std::shared_ptr<mtd::StubAndroidNativeBuffer> stub_native_buffer;
    mtd::StubSwappingGLContext stub_context;
    testing::NiceMock<mtd::MockSwappingGLContext> mock_context;
    mtd::StubRenderableListCompositor stub_compositor;
    mga::DisplayName primary{mga::DisplayName::primary};
    mga::LayerList list{std::make_shared<mga::Hwc10Adapter>(), {}, geom::Displacement{}};
    hwc_layer_1_t skip_layer;
};
}

TEST_F(HwcFbDevice, hwc10_subscribes_to_vsync_events)
{
    using namespace testing;
    Sequence seq;
    EXPECT_CALL(*mock_hwc_device_wrapper, subscribe_to_events(_,_,_,_))
        .InSequence(seq);
    EXPECT_CALL(*mock_hwc_device_wrapper, unsubscribe_from_events_(_))
        .InSequence(seq);

    mga::HwcFbDevice device(mock_hwc_device_wrapper, mock_fb_device);
}

TEST_F(HwcFbDevice, hwc10_rejects_overlays)
{
    using namespace testing;
    mtd::StubRenderableListCompositor stub_compositor;
    auto renderable1 = std::make_shared<mtd::StubRenderable>();
    auto renderable2 = std::make_shared<mtd::StubRenderable>();
    mg::RenderableList renderlist
    {
        renderable1,
        renderable2
    };

    mga::HwcFbDevice device(mock_hwc_device_wrapper, mock_fb_device);
    EXPECT_FALSE(device.compatible_renderlist(renderlist));
}

TEST_F(HwcFbDevice, hwc10_post)
{
    using namespace testing;
    std::list<hwc_layer_1_t*> expected_list{&skip_layer};
    std::function<void(mga::DisplayName, std::chrono::nanoseconds)> vsync_cb;
    EXPECT_CALL(*mock_hwc_device_wrapper, subscribe_to_events(_,_,_,_))
        .WillOnce(SaveArg<1>(&vsync_cb));
    mga::HwcFbDevice device(mock_hwc_device_wrapper, mock_fb_device);
    Mock::VerifyAndClearExpectations(mock_hwc_device_wrapper.get());

    std::atomic<bool> vsync_thread_on{true};
    mir::test::AutoUnblockThread vsync_thread(
        [&]{ vsync_thread_on = false; },
        [&]{
            while(vsync_thread_on)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                vsync_cb(mga::DisplayName::primary, std::chrono::nanoseconds(0));
            }});

    Sequence seq;
    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .InSequence(seq)
        .WillOnce(Return(stub_native_buffer));
    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(MatchesPrimaryList(expected_list)))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq)
        .WillOnce(Return(dpy));
    EXPECT_CALL(mock_egl, eglGetCurrentSurface(EGL_DRAW))
        .InSequence(seq)
        .WillOnce(Return(sur));
    EXPECT_CALL(*mock_hwc_device_wrapper, set(MatchesListWithEglFields(expected_list, dpy, sur)))
        .InSequence(seq);
    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .InSequence(seq)
        .WillOnce(Return(stub_native_buffer));
    EXPECT_CALL(*mock_fb_device, post_interface(mock_fb_device.get(), &stub_native_buffer->native_handle))
        .InSequence(seq);

    mga::DisplayContents content{primary, list, geom::Displacement{}, mock_context, stub_compositor};
    device.commit({content});

    // Predictive bypass not enabled in HwcFbDevice
    EXPECT_EQ(0, device.recommended_sleep().count());
}
