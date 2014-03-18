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
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/mock_render_function.h"
#include "mir_test_doubles/stub_swapping_gl_context.h"
#include "mir_test_doubles/mock_egl.h"
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

    int fake_dpy = 0;
    int fake_sur = 0;
    EGLDisplay dpy{&fake_dpy};
    EGLSurface sur{&fake_sur};

    testing::NiceMock<mtd::MockEGL> mock_egl;

    MirPixelFormat test_pf;
    geom::Size test_size;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_hwc_device;
    std::shared_ptr<mtd::MockFBHalDevice> mock_fb_device;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    mtd::StubSwappingGLContext stub_context;
};

TEST_F(HwcFbDevice, hwc10_render_gl_only)
{
    using namespace testing;
    Sequence seq;
    EXPECT_CALL(*mock_hwc_device, prepare_interface(mock_hwc_device.get(), 1, _))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq)
        .WillOnce(Return(dpy));
    EXPECT_CALL(mock_egl, eglGetCurrentSurface(EGL_DRAW))
        .InSequence(seq)
        .WillOnce(Return(sur));
    EXPECT_CALL(*mock_hwc_device, set_interface(mock_hwc_device.get(), 1, _))
        .InSequence(seq);

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    device.render_gl(stub_context);

    //prepare expectations
    EXPECT_THAT(mock_hwc_device->display0_prepare_content.retireFenceFd, Eq(-1));
    EXPECT_THAT(mock_hwc_device->display0_prepare_content.flags, Eq(HWC_GEOMETRY_CHANGED)); 
    EXPECT_THAT(mock_hwc_device->display0_prepare_content.numHwLayers, Eq(1u));
    ASSERT_THAT(mock_hwc_device->display0_prepare_content.hwLayers, Ne(nullptr));
    EXPECT_THAT(mock_hwc_device->prepare_layerlist[0].compositionType, Eq(HWC_FRAMEBUFFER));
    EXPECT_THAT(mock_hwc_device->prepare_layerlist[0].flags, Eq(HWC_SKIP_LAYER));

    //set expectations
    EXPECT_THAT(mock_hwc_device->display0_set_content.dpy, Eq(dpy));
    EXPECT_THAT(mock_hwc_device->display0_set_content.sur, Eq(sur));
    EXPECT_THAT(mock_hwc_device->display0_set_content.retireFenceFd, Eq(-1));
    EXPECT_THAT(mock_hwc_device->display0_set_content.flags, Eq(HWC_GEOMETRY_CHANGED));
    EXPECT_THAT(mock_hwc_device->display0_set_content.numHwLayers, Eq(1u));
    ASSERT_THAT(mock_hwc_device->display0_set_content.hwLayers, Ne(nullptr));
    EXPECT_THAT(mock_hwc_device->set_layerlist[0].compositionType, Eq(HWC_FRAMEBUFFER));
    EXPECT_THAT(mock_hwc_device->set_layerlist[0].flags, Eq(HWC_SKIP_LAYER));
}

TEST_F(HwcFbDevice, hwc10_prepare_with_renderables)
{
    using namespace testing;
    auto renderable1 = std::make_shared<mtd::StubRenderable>();
    auto renderable2 = std::make_shared<mtd::StubRenderable>();
    std::list<std::shared_ptr<mg::Renderable>> renderlist
    {
        renderable1,
        renderable2
    };

    mtd::MockRenderFunction mock_call_counter;
    testing::Sequence seq;
    EXPECT_CALL(*mock_hwc_device, prepare_interface(mock_hwc_device.get(), 1, _))
        .InSequence(seq);
    EXPECT_CALL(mock_call_counter, called(testing::Ref(*renderable1)))
        .InSequence(seq);
    EXPECT_CALL(mock_call_counter, called(testing::Ref(*renderable2)))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .InSequence(seq)
        .WillOnce(Return(dpy));
    EXPECT_CALL(mock_egl, eglGetCurrentSurface(EGL_DRAW))
        .InSequence(seq)
        .WillOnce(Return(sur));
    EXPECT_CALL(*mock_hwc_device, set_interface(mock_hwc_device.get(), 1, _))
        .InSequence(seq);

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    device.render_gl_and_overlays(stub_context, renderlist, [&](mg::Renderable const& renderable)
    {
        mock_call_counter.called(renderable);
    });

    //prepare expectations
    EXPECT_THAT(mock_hwc_device->display0_prepare_content.retireFenceFd, Eq(-1));
    EXPECT_THAT(mock_hwc_device->display0_prepare_content.flags, Eq(HWC_GEOMETRY_CHANGED)); 
    EXPECT_THAT(mock_hwc_device->display0_prepare_content.numHwLayers, Eq(1u));
    ASSERT_THAT(mock_hwc_device->display0_prepare_content.hwLayers, Ne(nullptr));
    EXPECT_THAT(mock_hwc_device->prepare_layerlist[0].compositionType, Eq(HWC_FRAMEBUFFER));
    EXPECT_THAT(mock_hwc_device->prepare_layerlist[0].flags, Eq(HWC_SKIP_LAYER));

    //set expectations
    EXPECT_THAT(mock_hwc_device->display0_set_content.dpy, Eq(dpy));
    EXPECT_THAT(mock_hwc_device->display0_set_content.sur, Eq(sur));
    EXPECT_THAT(mock_hwc_device->display0_set_content.retireFenceFd, Eq(-1));
    EXPECT_THAT(mock_hwc_device->display0_set_content.flags, Eq(HWC_GEOMETRY_CHANGED));
    EXPECT_THAT(mock_hwc_device->display0_set_content.numHwLayers, Eq(1u));
    ASSERT_THAT(mock_hwc_device->display0_set_content.hwLayers, Ne(nullptr));
    EXPECT_THAT(mock_hwc_device->set_layerlist[0].compositionType, Eq(HWC_FRAMEBUFFER));
    EXPECT_THAT(mock_hwc_device->set_layerlist[0].flags, Eq(HWC_SKIP_LAYER));
}

TEST_F(HwcFbDevice, hwc10_prepare_frame_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_hwc_device, prepare_interface(mock_hwc_device.get(), _, _))
        .Times(1)
        .WillOnce(Return(-1));

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    EXPECT_THROW({
        device.render_gl(stub_context);
    }, std::runtime_error);
}

TEST_F(HwcFbDevice, hwc10_commit_frame_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_hwc_device, set_interface(mock_hwc_device.get(), _, _))
        .Times(1)
        .WillOnce(Return(-1));

    mga::HwcFbDevice device(mock_hwc_device, mock_fb_device, mock_vsync);

    EXPECT_THROW({
        device.render_gl(stub_context);
    }, std::runtime_error);
}
