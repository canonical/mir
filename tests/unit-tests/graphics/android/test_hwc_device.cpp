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
#include "mir_test_doubles/mock_hwc_composer_device_1.h"

#include <thread>
#include <chrono>
#include <stdexcept>
#include <memory>
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

class HWCDevice : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
    }

    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
};

TEST_F(HWCDevice, test_proc_registration)
{
    using namespace testing;

    hwc_procs_t const* procs;
    EXPECT_CALL(*mock_device, registerProcs_interface(mock_device.get(), _))
        .Times(1)
        .WillOnce(SaveArg<1>(&procs));

    mga::HWC11Device device(mock_device);

    EXPECT_NE(nullptr, procs->invalidate);
    EXPECT_NE(nullptr, procs->vsync);
    EXPECT_NE(nullptr, procs->hotplug);
}

TEST_F(HWCDevice, test_vsync_activation_comes_after_proc_registration)
{
    using namespace testing;

    InSequence sequence_enforcer;
    EXPECT_CALL(*mock_device, registerProcs_interface(mock_device.get(),_))
        .Times(1);
    EXPECT_CALL(*mock_device, eventControl_interface(mock_device.get(), 0, HWC_EVENT_VSYNC, 1))
        .Times(1)
        .WillOnce(Return(0));

    mga::HWC11Device device(mock_device);
    testing::Mock::VerifyAndClearExpectations(mock_device.get());
}

TEST_F(HWCDevice, test_vsync_activation_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(*mock_device, eventControl_interface(mock_device.get(), 0, HWC_EVENT_VSYNC, 1))
        .Times(1)
        .WillOnce(Return(-EINVAL));

    EXPECT_THROW({
        mga::HWC11Device device(mock_device);
    }, std::runtime_error);
}

namespace
{
static mga::HWC11Device *global_device;
void* waiting_device(void*)
{
    global_device->wait_for_vsync();
    return NULL;
}
}

TEST_F(HWCDevice, test_vsync_hook_waits)
{
    mga::HWC11Device device(mock_device);
    global_device = &device;

    pthread_t thread;
    pthread_create(&thread, NULL, waiting_device, NULL);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    void* retval;
    auto error = pthread_tryjoin_np(thread, &retval);
    ASSERT_EQ(EBUSY, error);

    device.notify_vsync();
    error = pthread_join(thread, &retval);
    ASSERT_EQ(0, error);

}

TEST_F(HWCDevice, test_vsync_hook_from_hwc_unblocks_wait)
{
    using namespace testing;

    hwc_procs_t const* procs;
    EXPECT_CALL(*mock_device, registerProcs_interface(mock_device.get(), _))
        .Times(1)
        .WillOnce(SaveArg<1>(&procs));

    mga::HWC11Device device(mock_device);
    global_device = &device;

    pthread_t thread;
    pthread_create(&thread, NULL, waiting_device, NULL);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    void* retval;
    auto error = pthread_tryjoin_np(thread, &retval);
    ASSERT_EQ(EBUSY, error);

    procs->vsync(procs, 0, 0);
    error = pthread_join(thread, &retval);
    ASSERT_EQ(0, error);
}

TEST_F(HWCDevice, test_hwc_gles_set)
{
    using namespace testing;

    mga::HWC11Device device(mock_device);

    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), HWC_NUM_DISPLAY_TYPES, _))
        .Times(1);

    device.commit_frame();

    EXPECT_EQ(0u, mock_device->display0_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_content.retireFenceFd);
}

TEST_F(HWCDevice, test_hwc_gles_set_error)
{
    using namespace testing;

    mga::HWC11Device device(mock_device);
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), HWC_NUM_DISPLAY_TYPES, _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        device.commit_frame();
    }, std::runtime_error);
}

TEST_F(HWCDevice, test_hwc_turns_on_display_after_proc_registration)
{
    using namespace testing;
    InSequence sequence_enforcer;
    EXPECT_CALL(*mock_device, registerProcs_interface(mock_device.get(),_))
        .Times(1);
    EXPECT_CALL(*mock_device, blank_interface(mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(1);

    mga::HWC11Device device(mock_device);
    testing::Mock::VerifyAndClearExpectations(mock_device.get());
}

TEST_F(HWCDevice, test_hwc_throws_on_blank_error)
{
    using namespace testing;

    EXPECT_CALL(*mock_device, blank_interface(mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mga::HWC11Device device(mock_device);
    }, std::runtime_error);
}

TEST_F(HWCDevice, test_hwc_display_is_deactivated_on_destroy)
{
    auto device = std::make_shared<mga::HWC11Device>(mock_device);

    EXPECT_CALL(*mock_device, blank_interface(mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(1);
    EXPECT_CALL(*mock_device, eventControl_interface(mock_device.get(), HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, 0))
        .Times(1);
    device.reset();
}

TEST_F(HWCDevice, test_hwc_device_display_config)
{
    using namespace testing;

    unsigned int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,Pointee(1)))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));

    mga::HWC11Device device(mock_device);
}

//apparently this can happen if the display is in the 'unplugged state'
TEST_F(HWCDevice, test_hwc_device_display_config_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mga::HWC11Device device(mock_device);
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

TEST_F(HWCDevice, test_hwc_device_display_width_height)
{
    using namespace testing;

    int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));

    mga::HWC11Device device(mock_device);
 
    EXPECT_CALL(*mock_device, getDisplayAttributes_interface(mock_device.get(), HWC_DISPLAY_PRIMARY,hwc_configs,_,_))
        .Times(1)
        .WillOnce(Invoke(display_attribute_handler));

    auto size = device.display_size();
    EXPECT_EQ(size.width.as_uint32_t(),  static_cast<unsigned int>(display_width));
    EXPECT_EQ(size.height.as_uint32_t(), static_cast<unsigned int>(display_height));
}
}
