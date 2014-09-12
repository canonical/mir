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

#include "mir/graphics/android/sync_fence.h"
#include "src/platform/graphics/android/hwc_fb_device.h"
#include "src/platform/graphics/android/hwc_device.h"
#include "src/platform/graphics/android/hwc_wrapper.h"
#include "src/platform/graphics/android/hwc_layerlist.h"
#include "src/platform/graphics/android/hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_hwc_device_wrapper.h"
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
    std::shared_ptr<mga::HwcWrapper> const& hwc_device,
    std::shared_ptr<framebuffer_device_t> const& fb_device,
    std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator);

template <>
std::shared_ptr<mga::HWCCommonDevice> make_hwc_device<mga::HwcFbDevice>(
    std::shared_ptr<mga::HwcWrapper> const& hwc_device,
    std::shared_ptr<framebuffer_device_t> const& fb_device,
    std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator)
{
    return std::make_shared<mga::HwcFbDevice>(hwc_device, fb_device, coordinator);
}

template <>
std::shared_ptr<mga::HWCCommonDevice> make_hwc_device<mga::HwcDevice>(
    std::shared_ptr<mga::HwcWrapper> const& hwc_device,
    std::shared_ptr<framebuffer_device_t> const&,
    std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator)
{
    auto file_ops = std::make_shared<mga::RealSyncFileOps>();
    return std::make_shared<mga::HwcDevice>(hwc_device, coordinator, file_ops);
}

template<typename T>
class HWCCommon : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_fbdev = std::make_shared<mtd::MockFBHalDevice>();
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCDeviceWrapper>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCDeviceWrapper> mock_device;
    std::shared_ptr<mtd::MockFBHalDevice> mock_fbdev;
};

typedef ::testing::Types<mga::HwcFbDevice, mga::HwcDevice> HWCDeviceTestTypes;
TYPED_TEST_CASE(HWCCommon, HWCDeviceTestTypes);

TYPED_TEST(HWCCommon, test_proc_registration)
{
    using namespace testing;
    std::shared_ptr<mga::HWCCallbacks> callbacks;
    EXPECT_CALL(*(this->mock_device), register_hooks(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&callbacks));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

    ASSERT_THAT(callbacks, Ne(nullptr));
    EXPECT_THAT(callbacks->hooks.invalidate, Ne(nullptr));
    EXPECT_THAT(callbacks->hooks.vsync, Ne(nullptr));
    EXPECT_THAT(callbacks->hooks.hotplug, Ne(nullptr));
}

TYPED_TEST(HWCCommon, test_device_destruction_unregisters_self_from_hooks)
{
    using namespace testing;
    std::shared_ptr<mga::HWCCallbacks> callbacks;
    EXPECT_CALL(*(this->mock_device), register_hooks(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&callbacks));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

    ASSERT_THAT(callbacks, Ne(nullptr));
    EXPECT_THAT(callbacks->self, Eq(device.get()));
    device = nullptr;
    EXPECT_THAT(callbacks->self, Eq(nullptr));    
}

TYPED_TEST(HWCCommon, registerst_hooks_and_turns_on_display)
{
    using namespace testing;

    Sequence seq;
    EXPECT_CALL(*this->mock_device, register_hooks(_))
        .InSequence(seq);
    EXPECT_CALL(*this->mock_device, display_on())
        .InSequence(seq);
    EXPECT_CALL(*this->mock_device, vsync_signal_on())
        .InSequence(seq);

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    testing::Mock::VerifyAndClearExpectations(this->mock_device.get());
}

TYPED_TEST(HWCCommon, test_hwc_suspend_standby_throw)
{
    using namespace testing;
    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

    EXPECT_THROW({
        device->mode(mir_power_mode_suspend);
    }, std::runtime_error);
    EXPECT_THROW({
        device->mode(mir_power_mode_standby);
    }, std::runtime_error);
}

TYPED_TEST(HWCCommon, test_hwc_deactivates_vsync_on_blank)
{
    using namespace testing;

    InSequence seq;
    EXPECT_CALL(*this->mock_device, display_on())
        .Times(1);
    EXPECT_CALL(*this->mock_device, vsync_signal_on())
        .Times(1);
    EXPECT_CALL(*this->mock_device, vsync_signal_off())
        .Times(1);
    EXPECT_CALL(*this->mock_device, display_off())
        .Times(1);

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    device->mode(mir_power_mode_off);
}

TYPED_TEST(HWCCommon, test_hwc_display_is_deactivated_on_destroy)
{
    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

    testing::InSequence seq;
    EXPECT_CALL(*this->mock_device, vsync_signal_off())
        .Times(1);
    EXPECT_CALL(*this->mock_device, display_off())
        .Times(1);
    device.reset();
}

TYPED_TEST(HWCCommon, catches_exception_during_destruction)
{
    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    EXPECT_CALL(*this->mock_device, display_off())
        .WillOnce(testing::Throw(std::runtime_error("")));
    device.reset();
}

TYPED_TEST(HWCCommon, callback_calls_hwcvsync)
{
    using namespace testing;
    std::shared_ptr<mga::HWCCallbacks> callbacks;
    EXPECT_CALL(*(this->mock_device), register_hooks(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&callbacks));

    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);

    EXPECT_CALL(*this->mock_vsync, notify_vsync())
        .Times(1);
    ASSERT_THAT(callbacks, Ne(nullptr));
    callbacks->hooks.vsync(&callbacks->hooks, 0, 0);

    callbacks->self = nullptr;
    callbacks->hooks.vsync(&callbacks->hooks, 0, 0);
}

TYPED_TEST(HWCCommon, set_orientation)
{
    auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    EXPECT_FALSE(device->apply_orientation(mir_orientation_left));
}

TYPED_TEST(HWCCommon, first_unblank_failure_is_not_fatal) //lp:1345533
{
    ON_CALL(*this->mock_device, display_on())
        .WillByDefault(testing::Throw(std::runtime_error("error")));
    EXPECT_NO_THROW({
        auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    });
}

TYPED_TEST(HWCCommon, first_vsync_failure_is_not_fatal) //lp:1345533
{
    ON_CALL(*this->mock_device, vsync_signal_on())
        .WillByDefault(testing::Throw(std::runtime_error("error")));
    EXPECT_NO_THROW({
        auto device = make_hwc_device<TypeParam>(this->mock_device, this->mock_fbdev, this->mock_vsync);
    });
}
