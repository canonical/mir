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
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_buffer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace mir
{
namespace test
{
namespace doubles
{
struct MockFBBundle : public mga::FramebufferBundle
{
    MockFBBundle()
    {
        using namespace testing;
        ON_CALL(*this, last_rendered_buffer())
            .WillByDefault(Return(std::make_shared<mtd::StubBuffer>()));
    }
    MOCK_METHOD0(fb_format, geom::PixelFormat());
    MOCK_METHOD0(fb_size, geom::Size());
    MOCK_METHOD0(buffer_for_render, std::shared_ptr<mg::Buffer>());
    MOCK_METHOD0(last_rendered_buffer, std::shared_ptr<mg::Buffer>());
};
}
}
}

class HWC11Device : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_fb_bundle = std::make_shared<testing::NiceMock<mtd::MockFBBundle>>();
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
    }

    std::shared_ptr<mtd::MockFBBundle> mock_fb_bundle;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    EGLDisplay dpy;
    EGLSurface surf;
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(HWC11Device, test_hwc_commit_order)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_fb_bundle, mock_vsync);

    InSequence seq;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(1);
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1);
    EXPECT_CALL(*mock_fb_bundle, last_rendered_buffer())
        .Times(1);
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1);

    device.commit_frame(dpy, surf);

    EXPECT_EQ(2, mock_device->display0_prepare_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_prepare_content.retireFenceFd);
    //set
    EXPECT_EQ(2, mock_device->display0_set_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_set_content.retireFenceFd);
    EXPECT_EQ(HWC_FRAMEBUFFER_TARGET, mock_device->set_layerlist[1].compositionType);
    EXPECT_EQ(0, mock_device->set_layerlist[1].flags);
}

TEST_F(HWC11Device, test_hwc_commit_failure)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_fb_bundle, mock_vsync);

    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        device.commit_frame(dpy, surf);
    }, std::runtime_error);
}

TEST_F(HWC11Device, test_hwc_swapbuffers_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    mga::HWC11Device device(mock_device, mock_fb_bundle, mock_vsync);

    EXPECT_THROW({
        device.commit_frame(dpy, surf);
    }, std::runtime_error);
}
