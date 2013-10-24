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

#include "src/server/graphics/android/hwc10_device.h"
#include "src/server/graphics/android/hwc11_device.h"
#include "src/server/graphics/android/hwc_layerlist.h"
#include "src/server/graphics/android/hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_hwc_layerlist.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_display_commander.h"

#include <thread>
#include <chrono>
#include <stdexcept>
#include <memory>
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

template<class T>
std::shared_ptr<mga::HWCCommonCommand> make_hwc_device(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                                                std::shared_ptr<mga::HWCLayerList> const& layer_list,
                                                std::shared_ptr<mga::DisplayCommander> const& fbdev,
                                                std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator);

template <>
std::shared_ptr<mga::HWCCommonCommand> make_hwc_device<mga::HWC10Device>(
                                                std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                                                std::shared_ptr<mga::HWCLayerList> const&, 
                                                std::shared_ptr<mga::DisplayCommander> const& fbdev,
                                                std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator)
{
    return std::make_shared<mga::HWC10Device>(hwc_device, fbdev, coordinator);
}

template <>
std::shared_ptr<mga::HWCCommonCommand> make_hwc_device<mga::HWC11Device>(
                                                std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                                                std::shared_ptr<mga::HWCLayerList> const& layer_list,
                                                std::shared_ptr<mga::DisplayCommander> const& fbdev,
                                                std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator)
{
    return std::make_shared<mga::HWC11Device>(hwc_device, layer_list, fbdev, coordinator);
}

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

template<typename T>
class HWCCommon : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_layer_list = std::make_shared<testing::NiceMock<mtd::MockHWCLayerList>>();
        mock_fbdev = std::make_shared<testing::NiceMock<mtd::MockDisplayCommander>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
    }

    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCLayerList> mock_layer_list;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    std::shared_ptr<mtd::MockDisplayCommander> mock_fbdev;
};

typedef ::testing::Types<mga::HWC10Device, mga::HWC11Device> HWCDeviceTestTypes;
TYPED_TEST_CASE(HWCCommon, HWCDeviceTestTypes);

TYPED_TEST(HWCCommon, test_proc_registration)
{
    using namespace testing;

    hwc_procs_t const* procs;
    EXPECT_CALL(*(this->mock_device), registerProcs_interface(this->mock_device.get(), _))
        .Times(1)
        .WillOnce(SaveArg<1>(&procs));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);

    EXPECT_NE(nullptr, procs->invalidate);
    EXPECT_NE(nullptr, procs->vsync);
    EXPECT_NE(nullptr, procs->hotplug);
}

TYPED_TEST(HWCCommon, test_vsync_activation_comes_after_proc_registration)
{
    using namespace testing;

    InSequence sequence_enforcer;
    EXPECT_CALL(*this->mock_device, registerProcs_interface(this->mock_device.get(),_))
        .Times(1);
    EXPECT_CALL(*this->mock_device, eventControl_interface(this->mock_device.get(), 0, HWC_EVENT_VSYNC, 1))
        .Times(1)
        .WillOnce(Return(0));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);
    testing::Mock::VerifyAndClearExpectations(this->mock_device.get());
}

TYPED_TEST(HWCCommon, test_vsync_activation_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(*this->mock_device, eventControl_interface(this->mock_device.get(), 0, HWC_EVENT_VSYNC, 1))
        .Times(1)
        .WillOnce(Return(-EINVAL));

    EXPECT_THROW({
        auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                                 this->mock_fbdev, this->mock_vsync);
    }, std::runtime_error);
}

TYPED_TEST(HWCCommon, test_hwc_turns_on_display_after_proc_registration)
{
    using namespace testing;
    InSequence sequence_enforcer;
    EXPECT_CALL(*this->mock_device, registerProcs_interface(this->mock_device.get(),_))
        .Times(1);
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(1);

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);
    testing::Mock::VerifyAndClearExpectations(this->mock_device.get());
}

TYPED_TEST(HWCCommon, test_hwc_ensures_unblank_during_initialization)
{
    using namespace testing;

    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                                 this->mock_fbdev, this->mock_vsync);
    }, std::runtime_error);
}

TYPED_TEST(HWCCommon, test_hwc_throws_on_blanking_error)
{
    using namespace testing;

    InSequence seq;
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(1)
        .WillOnce(Return(0));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);
    EXPECT_THROW({
        device->mode(mir_power_mode_off);
    }, std::runtime_error);
}

TYPED_TEST(HWCCommon, test_hwc_suspend_standby_turn_off)
{
    using namespace testing;

    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(3)
        .WillOnce(Return(0));
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(3)
        .WillOnce(Return(0));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);
    device->mode(mir_power_mode_off);
    device->mode(mir_power_mode_on);
    device->mode(mir_power_mode_suspend);
    device->mode(mir_power_mode_on);
    device->mode(mir_power_mode_standby);
}

TYPED_TEST(HWCCommon, test_hwc_deactivates_vsync_on_blank)
{
    using namespace testing;

    InSequence seq;
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(*this->mock_device, eventControl_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, 1))
        .Times(1);

    EXPECT_CALL(*this->mock_device, eventControl_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, 0))
        .Times(1);
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(1)
        .WillOnce(Return(0));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);
    device->mode(mir_power_mode_off);
}

TYPED_TEST(HWCCommon, test_blank_is_ignored_if_already_in_correct_state)
{
    using namespace testing;

    //we start off unblanked

    InSequence seq;
    //from constructor
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(Exactly(1))
        .WillOnce(Return(0));
     //from destructor
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(1)
        .WillOnce(Return(0));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);
    device->mode(mir_power_mode_on);
}

TYPED_TEST(HWCCommon, test_hwc_display_is_deactivated_on_destroy)
{
    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);

    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(1);
    EXPECT_CALL(*this->mock_device, eventControl_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, 0))
        .Times(1);
    device.reset();
}

TYPED_TEST(HWCCommon, callback_calls_hwcvsync)
{
    using namespace testing;

    hwc_procs_t const* procs;
    EXPECT_CALL(*this->mock_device, registerProcs_interface(this->mock_device.get(), _))
        .Times(1)
        .WillOnce(SaveArg<1>(&procs));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_layer_list,
                                             this->mock_fbdev, this->mock_vsync);

    EXPECT_CALL(*this->mock_vsync, notify_vsync())
        .Times(1);
    procs->vsync(procs, 0, 0);
}

#if 0
TYPED_TEST(HWCommon, test_hwc_device_display_config)
{
    using namespace testing;

    unsigned int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,Pointee(1)))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));

    mga::HWCInfo device(mock_device);
}

//apparently this can happen if the display is in the 'unplugged state'
TYPED_TEST(HWCommon, test_hwc_device_display_config_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mga::HWCInfo device(mock_device);
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

TYPED_TEST(HWCommon, test_hwc_device_display_width_height)
{
    using namespace testing;

    int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));

    mga::HWCInfo device(mock_device);
 
    EXPECT_CALL(*mock_device, getDisplayAttributes_interface(mock_device.get(), HWC_DISPLAY_PRIMARY,hwc_configs,_,_))
        .Times(1)
        .WillOnce(Invoke(display_attribute_handler));

    auto size = device.display_size();
    EXPECT_EQ(size.width.as_uint32_t(),  static_cast<unsigned int>(display_width));
    EXPECT_EQ(size.height.as_uint32_t(), static_cast<unsigned int>(display_height));
}

TYPED_TEST(HWCommon, hwc_device_reports_2_fbs_available_by_default)
{
    mga::HWCInfo device(mock_device);
    EXPECT_EQ(2u, device.number_of_framebuffers_available());
}

TYPED_TEST(HWCommon, hwc_device_reports_abgr_8888_by_default)
{
    mga::HWCInfo device(mock_device);
    EXPECT_EQ(geom::PixelFormat::abgr_8888, device.display_format());
}
#endif
