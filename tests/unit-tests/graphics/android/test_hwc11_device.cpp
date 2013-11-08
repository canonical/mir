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

#include "src/server/graphics/android/hwc11_device.h"
#include "src/server/graphics/android/hwc_layerlist.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_egl.h"
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
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_display_device = std::make_shared<testing::NiceMock<mtd::MockDisplayDevice>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
    }

    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    std::shared_ptr<mtd::MockDisplayDevice> mock_display_device;
    EGLDisplay dpy;
    EGLSurface surf;
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(HWC11Device, test_hwc_gles_set_empty_layerlist)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);

    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1);
    device.commit_frame(dpy, surf);

    EXPECT_EQ(1, mock_device->display0_set_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_set_content.retireFenceFd);
}

TEST_F(HWC11Device, test_hwc_gles_set_error)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);

    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        device.commit_frame(dpy, surf);
    }, std::runtime_error);
}

TEST_F(HWC11Device, test_hwc_gles_commit_swapbuffers_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);

    EXPECT_THROW({
        device.commit_frame(dpy, surf);
    }, std::runtime_error);
}

TEST_F(HWC11Device, test_hwc_commit_order_with_vsync)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);

    //the order here is very important. eglSwapBuffers will alter the layerlist,
    //so it must come before assembling the data for set
    InSequence seq;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(1);
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1);
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1);

    device.commit_frame(dpy, surf);

    EXPECT_EQ(1, mock_device->display0_prepare_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_prepare_content.retireFenceFd);
    EXPECT_EQ(1, mock_device->display0_set_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_set_content.retireFenceFd);
}

TEST_F(HWC11Device, test_hwc_device_display_config)
{
    using namespace testing;

    unsigned int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,Pointee(1)))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));

    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);
}


//apparently this can happen if the display is in the 'unplugged state'
TEST_F(HWC11Device, test_hwc_device_display_config_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);
    }, std::runtime_error);
}

namespace
{
static int const display_width = 180;
static int const display_height = 1010101;

static int display_attribute_handler(struct hwc_composer_device_1*, int, uint32_t,
                                     const uint32_t* attribute_list, int32_t* values)
{
    EXPECT_EQ(attribute_list[0], HWC_DISPLAY_WIDTH);
    EXPECT_EQ(attribute_list[1], HWC_DISPLAY_HEIGHT);
    EXPECT_EQ(attribute_list[2], HWC_DISPLAY_NO_ATTRIBUTE);

    values[0] = display_width;
    values[1] = display_height;
    return 0;
}
}

TEST_F(HWC11Device, test_hwc_device_display_width_height)
{
    using namespace testing;

    int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));
    EXPECT_CALL(*mock_device, getDisplayAttributes_interface(mock_device.get(), HWC_DISPLAY_PRIMARY,hwc_configs,_,_))
        .Times(1)
        .WillOnce(Invoke(display_attribute_handler));

    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);
 
    auto size = device.display_size();
    EXPECT_EQ(size.width.as_uint32_t(),  static_cast<unsigned int>(display_width));
    EXPECT_EQ(size.height.as_uint32_t(), static_cast<unsigned int>(display_height));
}

TEST_F(HWC11Device, hwc_device_reports_2_fbs_available_by_default)
{
    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);
    EXPECT_EQ(2u, device.number_of_framebuffers_available());
}

TEST_F(HWC11Device, hwc_version_11_format_selection)
{
    using namespace testing;
    EGLint const expected_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_FRAMEBUFFER_TARGET_ANDROID, EGL_TRUE,
        EGL_NONE
    };

    int visual_id = HAL_PIXEL_FORMAT_BGRA_8888;
    EGLDisplay fake_display = reinterpret_cast<EGLDisplay>(0x11235813);
    EGLConfig fake_egl_config = reinterpret_cast<EGLConfig>(0x44);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .InSequence(seq)
        .WillOnce(Return(fake_display));
    EXPECT_CALL(mock_egl, eglInitialize(fake_display,_,_))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglChooseConfig(fake_display,mtd::AttrMatches(expected_egl_config_attr),_,1,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<2>(fake_egl_config), SetArgPointee<4>(1), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(fake_display, fake_egl_config, EGL_NATIVE_VISUAL_ID, _))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<3>(visual_id), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglTerminate(fake_display))
        .InSequence(seq);
 
    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);
    EXPECT_EQ(geom::PixelFormat::argb_8888, device.display_format());
}

//not all hwc11 implementations give a hint about their framebuffer formats in their configuration.
//prefer abgr_8888 if we can't figure things out
TEST_F(HWC11Device, hwc_version_11_format_selection_failure)
{
    using namespace testing;
    EGLDisplay fake_display = reinterpret_cast<EGLDisplay>(0x11235813);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .InSequence(seq)
        .WillOnce(Return(fake_display));
    EXPECT_CALL(mock_egl, eglInitialize(fake_display,_,_))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglChooseConfig(_,_,_,_,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<4>(0), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglTerminate(fake_display))
        .InSequence(seq);
    
    mga::HWC11Device device(mock_device, mock_display_device, mock_vsync);
    EXPECT_EQ(geom::PixelFormat::abgr_8888, device.display_format());
}
