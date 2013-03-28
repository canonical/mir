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

#include "src/server/graphics/android/android_fb_factory.h"
#include "src/server/graphics/android/hwc_device.h"
#include "src/server/graphics/android/hwc_factory.h"

#include "mir_test/hw_mock.h"
#include "mir_test_doubles/null_display.h"

#include <hardware/hwcomposer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

struct MockHWCFactory: public mga::HWCFactory
{
    MockHWCFactory()
    {
        ON_CALL(*this, create_hwc_1_1())
            .WillByDefault(testing::Return(std::shared_ptr<mga::HWCDevice>()));
    }
    MOCK_CONST_METHOD0(create_hwc_1_1, std::shared_ptr<mga::HWCDevice>());
};

#if 0
//this is now the class we're testing! 
struct MockFbFactory : public mga::FBFactory
{
    MockFbFactory()
    {
        using namespace testing;
        ON_CALL(*this, can_create_hwc_display())
            .WillByDefault(Return(true));
        ON_CALL(*this, create_hwc_display())
            .WillByDefault(Return(std::make_shared<mtd::NullDisplay>()));
        ON_CALL(*this, create_gpu_display())
            .WillByDefault(Return(std::make_shared<mtd::NullDisplay>()));
    }

    MOCK_CONST_METHOD0(can_create_hwc_display, bool());
    MOCK_CONST_METHOD0(create_hwc_display, std::shared_ptr<mg::Display>());
    MOCK_CONST_METHOD0(create_gpu_display, std::shared_ptr<mg::Display>());
 
};
#endif

class AndroidFramebufferSelectorTest : public ::testing::Test
{
public:
    AndroidFramebufferSelectorTest()
        : mock_hwc_factory(std::make_shared<MockHWCFactory>())
    {
    }

    void TearDown()
    {
        EXPECT_TRUE(hw_access_mock.open_count_matches_close());
    }
    std::shared_ptr<MockHWCFactory> mock_hwc_factory;
    mt::HardwareAccessMock hw_access_mock;
};

TEST_F(AndroidFramebufferSelectorTest, hwc_selection_gets_hwc_device)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::AndroidFBFactory fb_factory(mock_hwc_factory); 
}

/* this case occurs when the libhardware library is not found/malformed */
TEST_F(AndroidFramebufferSelectorTest, hwc_module_unavailble_always_creates_gpu_display)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(*mock_hwc_factory, create_hwc_1_1())
        .Times(0);

    mga::AndroidFBFactory fb_factory(mock_hwc_factory); 
    EXPECT_FALSE(fb_factory.can_create_hwc_display());
    EXPECT_THROW({
        fb_factory.create_hwc_display();
    }, std::runtime_error);
}

/* this is normal operation on hwc capable device */
TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_version_11_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_hwc_factory, create_hwc_1_1())
        .Times(1);

    mga::AndroidFBFactory fb_factory(mock_hwc_factory); 
    EXPECT_TRUE(fb_factory.can_create_hwc_display());
    fb_factory.create_hwc_display();
}


// TODO: kdub support v 1.0 and 1.2
TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_failure_because_hwc_version10_not_supported)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;

    EXPECT_CALL(*mock_hwc_factory, create_hwc_1_1())
        .Times(0);

    mga::AndroidFBFactory fb_factory(mock_hwc_factory); 

    EXPECT_FALSE(fb_factory.can_create_hwc_display());
    EXPECT_THROW({
        fb_factory.create_hwc_display();
    }, std::runtime_error);
}

TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_failure_because_hwc_version12_not_supported)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_2;

    EXPECT_CALL(*mock_hwc_factory, create_hwc_1_1())
        .Times(0);

    mga::AndroidFBFactory fb_factory(mock_hwc_factory); 

    EXPECT_FALSE(fb_factory.can_create_hwc_display());
    EXPECT_THROW({
        fb_factory.create_hwc_display();
    }, std::runtime_error);
}
