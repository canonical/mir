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

#include "src/server/graphics/android/framebuffer_bundle.h"
#include "src/server/graphics/android/hwc11_device.h"
#include "src/server/graphics/android/hwc_layerlist.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/stub_buffer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class HWC11Device : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_native_buffer = std::make_shared<testing::NiceMock<mtd::MockAndroidNativeBuffer>>();
        mock_buffer = std::make_shared<testing::NiceMock<mtd::MockBuffer>>();
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();

        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(mock_native_buffer));
    }
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> mock_native_buffer;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    EGLDisplay dpy;
    EGLSurface surf;
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(HWC11Device, test_hwc_prepare)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(1);

    mga::HWC11Device device(mock_device, mock_vsync);
    device.prepare_composition();
    EXPECT_EQ(2, mock_device->display0_prepare_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_prepare_content.retireFenceFd);
}

TEST_F(HWC11Device, test_hwc_render)
{
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1);
    mga::HWC11Device device(mock_device, mock_vsync);
    device.gpu_render(dpy, surf);
}

TEST_F(HWC11Device, test_hwc_swapbuffers_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    mga::HWC11Device device(mock_device, mock_vsync);

    EXPECT_THROW({
        device.gpu_render(dpy, surf);
    }, std::runtime_error);
}

TEST_F(HWC11Device, test_hwc_commit)
{
    using namespace testing;
    int hwc_return_fence = 94;
    mock_device->hwc_set_return_fence(hwc_return_fence);

    mga::HWC11Device device(mock_device, mock_vsync);

    InSequence seq;
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1);
    EXPECT_CALL(*mock_native_buffer, update_fence(hwc_return_fence))
        .Times(1);

    device.post(*mock_buffer);

    //set
    EXPECT_EQ(2, mock_device->display0_set_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_set_content.retireFenceFd);
    EXPECT_EQ(HWC_FRAMEBUFFER, mock_device->set_layerlist[0].compositionType);
    EXPECT_EQ(HWC_SKIP_LAYER, mock_device->set_layerlist[0].flags);
    EXPECT_EQ(HWC_FRAMEBUFFER_TARGET, mock_device->set_layerlist[1].compositionType);
    EXPECT_EQ(0, mock_device->set_layerlist[1].flags);
}

TEST_F(HWC11Device, test_hwc_commit_failure)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_vsync);

    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        device.post(*mock_buffer);
    }, std::runtime_error);
}
