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
#include "mir_test_doubles/mock_display_support_provider.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_hwc_organizer.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test/egl_mock.h"
#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

class HWC11Device : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_display_support_provider = std::make_shared<testing::NiceMock<mtd::MockDisplaySupportProvider>>();
        mock_organizer = std::make_shared<testing::NiceMock<mtd::MockHWCOrganizer>>();
        mock_egl.silence_uninteresting();
    }

    std::shared_ptr<mtd::MockHWCOrganizer> mock_organizer;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    std::shared_ptr<mtd::MockDisplaySupportProvider> mock_display_support_provider;
    EGLDisplay dpy;
    EGLSurface surf;
    mir::EglMock mock_egl;
};

namespace
{
struct HWCDummyLayer : public mga::HWCDefaultLayer
{
    HWCDummyLayer()
     : HWCDefaultLayer({})
    {
    }
};
}

TEST_F(HWC11Device, test_hwc_gles_set_empty_layerlist)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);

    mga::LayerList empty_list;
    EXPECT_CALL(*mock_organizer, native_list())
        .Times(1)
        .WillOnce(ReturnRef(empty_list));
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), HWC_NUM_DISPLAY_TYPES, _))
        .Times(1);

    device.commit_frame(dpy, surf);

    EXPECT_EQ(empty_list.size(), mock_device->display0_set_content.numHwLayers);

    EXPECT_EQ(-1, mock_device->display0_set_content.retireFenceFd);
}

TEST_F(HWC11Device, test_hwc_gles_set_gets_layerlist)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);

    mga::LayerList fb_list;
    fb_list.push_back(std::make_shared<HWCDummyLayer>());

    EXPECT_CALL(*mock_organizer, native_list())
        .Times(1)
        .WillOnce(ReturnRef(fb_list));
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), HWC_NUM_DISPLAY_TYPES, _))
        .Times(1);

    device.commit_frame(dpy, surf);

    EXPECT_EQ(fb_list.size(), mock_device->display0_set_content.numHwLayers);
}

TEST_F(HWC11Device, test_hwc_gles_set_error)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);
    mga::LayerList fb_list;
    fb_list.push_back(std::make_shared<HWCDummyLayer>());

    EXPECT_CALL(*mock_organizer, native_list())
        .Times(1)
        .WillOnce(ReturnRef(fb_list));
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), HWC_NUM_DISPLAY_TYPES, _))
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

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);

    EXPECT_THROW({
        device.commit_frame(dpy, surf);
    }, std::runtime_error);
}

TEST_F(HWC11Device, test_hwc_gles_set_commits_via_swapbuffers_then_set)
{
    using namespace testing;

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);

    mga::LayerList fb_list;
    fb_list.push_back(std::make_shared<HWCDummyLayer>());

    //the order here is very important. eglSwapBuffers will alter the layerlist,
    //so it must come before assembling the data for set
    InSequence seq;
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1);
    EXPECT_CALL(*mock_organizer, native_list())
        .Times(1)
        .WillOnce(ReturnRef(fb_list));
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), HWC_NUM_DISPLAY_TYPES, _))
        .Times(1);

    device.commit_frame(dpy, surf);

    EXPECT_EQ(fb_list.size(), mock_device->display0_set_content.numHwLayers);
}

TEST_F(HWC11Device, test_hwc_device_display_config)
{
    using namespace testing;

    unsigned int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,Pointee(1)))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);
}

//apparently this can happen if the display is in the 'unplugged state'
TEST_F(HWC11Device, test_hwc_device_display_config_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);
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

TEST_F(HWC11Device, test_hwc_device_display_width_height)
{
    using namespace testing;

    int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);
 
    EXPECT_CALL(*mock_device, getDisplayAttributes_interface(mock_device.get(), HWC_DISPLAY_PRIMARY,hwc_configs,_,_))
        .Times(1)
        .WillOnce(Invoke(display_attribute_handler));

    auto size = device.display_size();
    EXPECT_EQ(size.width.as_uint32_t(),  static_cast<unsigned int>(display_width));
    EXPECT_EQ(size.height.as_uint32_t(), static_cast<unsigned int>(display_height));
}
}

TEST_F(HWC11Device, hwc_device_set_next_frontbuffer_adds_to_layerlist)
{
    std::shared_ptr<mc::Buffer> mock_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*this->mock_organizer, set_fb_target(mock_buffer))
        .Times(1);
 
    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);
    device.set_next_frontbuffer(mock_buffer);
}

TEST_F(HWC11Device, hwc_device_set_next_frontbuffer_posts)
{
    std::shared_ptr<mc::Buffer> mock_buffer = std::make_shared<mtd::MockBuffer>();
    EXPECT_CALL(*this->mock_display_support_provider, set_next_frontbuffer(mock_buffer))
        .Times(1);

    mga::HWC11Device device(mock_device, mock_organizer, mock_display_support_provider);
    device.set_next_frontbuffer(mock_buffer);
}
