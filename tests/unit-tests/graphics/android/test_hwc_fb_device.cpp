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

#include "src/platform/graphics/android/hwc_fb_device.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/mock_fb_hal_device.h"
#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class HwcFbDevice : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        int width = 88;
        int height = 4;
        test_size = geom::Size{width, height};
        test_pf = mir_pixel_format_abgr_8888;
        int fbnum = 558;
        mock_hwc_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_fb_device = std::make_shared<mtd::MockFBHalDevice>(
            width, height, HAL_PIXEL_FORMAT_RGBA_8888, fbnum);
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();
    }

    MirPixelFormat test_pf;
    geom::Size test_size;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_hwc_device;
    std::shared_ptr<mtd::MockFBHalDevice> mock_fb_device;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
};

TEST_F(HwcFbDevice, hwc10_prepare_gl_only)
{
    using namespace testing;
    EXPECT_CALL(*mock_hwc_device, prepare_interface(mock_hwc_device.get(), 1, _))
        .Times(1);

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    device.prepare_gl();

    EXPECT_EQ(-1, mock_hwc_device->display0_prepare_content.retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, mock_hwc_device->display0_prepare_content.flags);
    EXPECT_EQ(1u, mock_hwc_device->display0_prepare_content.numHwLayers);
    ASSERT_NE(nullptr, mock_hwc_device->display0_prepare_content.hwLayers);
    EXPECT_EQ(HWC_FRAMEBUFFER, mock_hwc_device->prepare_layerlist[0].compositionType);
    EXPECT_EQ(HWC_SKIP_LAYER, mock_hwc_device->prepare_layerlist[0].flags);
}

TEST_F(HwcFbDevice, hwc10_prepare_with_renderables)
{
    using namespace testing;
    EXPECT_CALL(*mock_hwc_device, prepare_interface(mock_hwc_device.get(), 1, _))
        .Times(1);

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    std::list<std::shared_ptr<mg::Renderable>> renderlist;
    device.prepare_gl_and_overlays(renderlist);

    EXPECT_EQ(-1, mock_hwc_device->display0_prepare_content.retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, mock_hwc_device->display0_prepare_content.flags);
    EXPECT_EQ(1u, mock_hwc_device->display0_prepare_content.numHwLayers);
    ASSERT_NE(nullptr, mock_hwc_device->display0_prepare_content.hwLayers);
    EXPECT_EQ(HWC_FRAMEBUFFER, mock_hwc_device->prepare_layerlist[0].compositionType);
    EXPECT_EQ(HWC_SKIP_LAYER, mock_hwc_device->prepare_layerlist[0].flags);
}

TEST_F(HwcFbDevice, hwc10_render_frame)
{
    using namespace testing;

    int fake_dpy = 0;
    int fake_sur = 0;
    EGLDisplay dpy = &fake_dpy;
    EGLSurface sur = &fake_sur;

    EXPECT_CALL(*mock_hwc_device, set_interface(mock_hwc_device.get(), 1, _))
        .Times(1);

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    device.gpu_render(dpy, sur);

    EXPECT_EQ(dpy, mock_hwc_device->display0_set_content.dpy);
    EXPECT_EQ(sur, mock_hwc_device->display0_set_content.sur);
    EXPECT_EQ(-1, mock_hwc_device->display0_set_content.retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, mock_hwc_device->display0_set_content.flags);
    EXPECT_EQ(1u, mock_hwc_device->display0_set_content.numHwLayers);
    ASSERT_NE(nullptr, mock_hwc_device->display0_set_content.hwLayers);
    EXPECT_EQ(HWC_FRAMEBUFFER, mock_hwc_device->set_layerlist[0].compositionType);
    EXPECT_EQ(HWC_SKIP_LAYER, mock_hwc_device->set_layerlist[0].flags);
}

TEST_F(HwcFbDevice, hwc10_prepare_frame_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_hwc_device, prepare_interface(mock_hwc_device.get(), _, _))
        .Times(1)
        .WillOnce(Return(-1));

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    EXPECT_THROW({
        device.prepare_gl();
    }, std::runtime_error);
}

TEST_F(HwcFbDevice, hwc10_commit_frame_failure)
{
    using namespace testing;

    int fake_dpy = 0;
    int fake_sur = 0;
    EGLDisplay dpy = &fake_dpy;
    EGLSurface sur = &fake_sur;
    EXPECT_CALL(*mock_hwc_device, set_interface(mock_hwc_device.get(), _, _))
        .Times(1)
        .WillOnce(Return(-1));

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    EXPECT_THROW({
        device.gpu_render(dpy, sur);
    }, std::runtime_error);
}
