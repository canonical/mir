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

#include "src/server/graphics/android/fb_factory.h"
#include "src/server/graphics/android/android_display_selector.h"

#include "mir_test/hw_mock.h"
#include "mir_test_doubles/null_display.h"

#include <hardware/hwcomposer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

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

class AndroidFramebufferSelectorTest : public ::testing::Test
{
public:
    AndroidFramebufferSelectorTest()
        : mock_fb_factory(std::make_shared<testing::NiceMock<MockFbFactory>>())
    {
    }

    std::shared_ptr<MockFbFactory> mock_fb_factory;
    mt::HardwareAccessMock hw_access_mock;
};

TEST_F(AndroidFramebufferSelectorTest, hwc_selection_gets_hwc_device)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}

/* this case occurs when the libhardware library cannot find the right .so files */
TEST_F(AndroidFramebufferSelectorTest, hwc_module_unavailble_always_creates_gpu_display)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(*mock_fb_factory, can_create_hwc_display())
        .Times(0);
    EXPECT_CALL(*mock_fb_factory, create_gpu_display())
        .Times(1); 

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}

/* this is normal operation on hwc capable device */
TEST_F(AndroidFramebufferSelectorTest, creates_hwc_device_when_hwc_possible)
{
    using namespace testing;

    EXPECT_CALL(*mock_fb_factory, can_create_hwc_display())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_fb_factory, create_hwc_display())
        .Times(1); 

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}

/* this happens when we don't support the hwc version, or there's a problem creating the HWCDevice */
TEST_F(AndroidFramebufferSelectorTest, creates_gpu_device_when_hwc_not_possible)
{
    using namespace testing;

    EXPECT_CALL(*mock_fb_factory, can_create_hwc_display())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_fb_factory, create_gpu_display())
        .Times(1); 

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}

#if 0
class HWCFactoryTest : public ::testing::Test
{
public:
    HWCFactoryTest()
    {
    }
};

TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_version_11_success)
{
    using namespace testing;

    std::shared_ptr<hw_module_t> mock_hwc_module;

    mock_hwc_module.version = HWC_DEVICE_API_VERSION_1_0;
    EXPECT_CALL(mock_hwc_module, open_interface());
        .Times(1);
    EXPECT_CALL(mock_hwc_module, close_interface());
        .Times(1);

    mga::AndroidFBFactory fb_factory(mock_hwc_module); 

    EXPECT_TRUE(fb_factory.can_create_hwc_device());
    EXPECT_NE(nullptr, fb_factory.create_hwc_device());
}

TEST_F(AndroidFramebufferSelectorTest, hwc_device_failure_because_hwc_does_not_open)
{
    using namespace testing;

    std::shared_ptr<hw_module_t> mock_hwc_module;
    EXPECT_CALL(mock_hwc_module, open_interface())
        .Times(1)
        .WillOnce(Return(-1));;
    EXPECT_CALL(mock_hwc_module, close_interface())
        .Times(0);

    mga::AndroidFBFactory fb_factory(mock_hwc_module); 

    EXPECT_FALSE(fb_factory.can_create_hwc_device());
    EXPECT_THROW({
        fb_factory.create_hwc_device();
    });
}

// TODO: kdub support v 1.0 and 1.2
TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_failure_because_hwc_version10_not_supported)
{
    using namespace testing;

    std::shared_ptr<hw_module_t> mock_hwc_module;

    mock_hwc_module.version = HWC_DEVICE_API_VERSION_1_0;
    EXPECT_CALL(mock_hwc_module, open_interface());
        .Times(1);
    EXPECT_CALL(mock_hwc_module, close_interface());
        .Times(1);

    mga::AndroidFBFactory fb_factory(mock_hwc_module); 

    EXPECT_FALSE(fb_factory.can_create_hwc_device());
    EXPECT_THROW({
        fb_factory.create_hwc_device();
    });
}

TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_failure_because_hwc_version12_not_supported)
{
    using namespace testing;

    auto mock_hwc_device = std::make_shared<mtd::MockHWCComposerDevice1>();
    mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_2;

    std::shared_ptr<hw_module_t> mock_hwc_module;

    mock_hwc_module.version = HWC_DEVICE_API_VERSION_1_0;
    EXPECT_CALL(mock_hwc_module, open_interface());
        .Times(1);
    EXPECT_CALL(mock_hwc_module, close_interface());
        .Times(1);

    mga::AndroidFBFactory fb_factory(mock_hwc_module); 

    EXPECT_FALSE(fb_factory.can_create_hwc_device());
    EXPECT_THROW({
        fb_factory.create_hwc_device();
    });
}
#endif
