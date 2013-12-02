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

#include "src/platform/graphics/android/hwc10_device.h"
#include "src/platform/graphics/android/hwc11_device.h"
#include "src/platform/graphics/android/hwc_layerlist.h"
#include "src/platform/graphics/android/hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_fb_hal_device.h"

#include <thread>
#include <chrono>
#include <stdexcept>
#include <memory>
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

template<class T>
std::shared_ptr<mga::HWCCommonDevice> make_hwc_device(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<framebuffer_device_t> const& fb_device,
    std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator);

template <>
std::shared_ptr<mga::HWCCommonDevice> make_hwc_device<mga::HWC10Device>(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<framebuffer_device_t> const& fb_device,
    std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator)
{
    return std::make_shared<mga::HWC10Device>(hwc_device, fb_device, coordinator);
}

template <>
std::shared_ptr<mga::HWCCommonDevice> make_hwc_device<mga::HWC11Device>(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<framebuffer_device_t> const&,
    std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator)
{
    return std::make_shared<mga::HWC11Device>(hwc_device, coordinator);
}

template<typename T>
class HWCCommon : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_fbdev = std::make_shared<mtd::MockFBHalDevice>();
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    std::shared_ptr<mtd::MockFBHalDevice> mock_fbdev;
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

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

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

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    testing::Mock::VerifyAndClearExpectations(this->mock_device.get());
}

TYPED_TEST(HWCCommon, test_vsync_activation_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(*this->mock_device, eventControl_interface(this->mock_device.get(), 0, HWC_EVENT_VSYNC, _))
        .WillRepeatedly(Return(-EINVAL));


    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    EXPECT_THROW({
        auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
        device->mode(mir_power_mode_off);
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

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    testing::Mock::VerifyAndClearExpectations(this->mock_device.get());
}

TYPED_TEST(HWCCommon, test_hwc_throws_on_blanking_request_error)
{
    using namespace testing;

    InSequence seq;
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(2)
        .WillOnce(Return(-1))
        .WillOnce(Return(0));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    EXPECT_THROW({
        device->mode(mir_power_mode_off);
    }, std::runtime_error);
}

TYPED_TEST(HWCCommon, test_hwc_suspend_standby_turn_off)
{
    using namespace testing;

    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 0))
        .Times(3)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*this->mock_device, blank_interface(this->mock_device.get(), HWC_DISPLAY_PRIMARY, 1))
        .Times(3)
        .WillRepeatedly(Return(0));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
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

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
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

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    device->mode(mir_power_mode_on);
}

TYPED_TEST(HWCCommon, test_hwc_display_is_deactivated_on_destroy)
{
    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

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

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

    EXPECT_CALL(*this->mock_vsync, notify_vsync())
        .Times(1);
    procs->vsync(procs, 0, 0);
}
