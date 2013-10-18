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

#include "src/server/graphics/android/display_support_provider.h"
#include "src/server/graphics/android/android_display_factory.h"
#include "src/server/graphics/android/resource_factory.h"
#include "src/server/graphics/android/fb_device.h"

#include "mir_test_doubles/mock_display_support_provider.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_android_hw.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;

namespace
{
struct MockResourceFactory: public mga::DisplayResourceFactory
{
    ~MockResourceFactory() noexcept {}
    MockResourceFactory()
    {
        using namespace testing;
        ON_CALL(*this, create_hwc_1_1(_,_))
            .WillByDefault(Return(std::shared_ptr<mga::DisplaySupportProvider>()));
        ON_CALL(*this, create_hwc_1_0(_,_))
            .WillByDefault(Return(std::shared_ptr<mga::DisplaySupportProvider>()));
    }

    MOCK_CONST_METHOD2(create_hwc_1_1, std::shared_ptr<mga::DisplaySupportProvider>(std::shared_ptr<hwc_composer_device_1> const&,
                                                                       std::shared_ptr<mga::DisplaySupportProvider> const&));
    MOCK_CONST_METHOD2(create_hwc_1_0, std::shared_ptr<mga::DisplaySupportProvider>(std::shared_ptr<hwc_composer_device_1> const&,
                                                                       std::shared_ptr<mga::DisplaySupportProvider> const&));
    MOCK_CONST_METHOD0(create_fb_device, std::shared_ptr<mga::DisplaySupportProvider>());
 
    MOCK_CONST_METHOD2(create_display, std::shared_ptr<mg::Display>(
        std::shared_ptr<mga::DisplaySupportProvider> const&,
        std::shared_ptr<mg::DisplayReport> const&));
};

class AndroidDisplayFactoryTest : public ::testing::Test
{
public:
    AndroidDisplayFactoryTest()
    {
    }

    void SetUp()
    {
        using namespace testing;
        mock_resource_factory = std::make_shared<MockResourceFactory>();
        mock_fb_device = std::make_shared<testing::NiceMock<mtd::MockDisplaySupportProvider>>();
        mock_display_report = std::make_shared<testing::NiceMock<mtd::MockDisplayReport>>();

        ON_CALL(*mock_resource_factory, create_fb_device())
            .WillByDefault(Return(mock_fb_device));
        ON_CALL(*mock_resource_factory, create_hwc_1_0(_,_))
            .WillByDefault(Return(nullptr));
        ON_CALL(*mock_resource_factory, create_hwc_1_1(_,_))
            .WillByDefault(Return(nullptr));
        ON_CALL(*mock_resource_factory, create_display(_,_))
            .WillByDefault(Return(nullptr));
    }

    void TearDown()
    {
        EXPECT_TRUE(hw_access_mock.open_count_matches_close());
    }

    std::shared_ptr<MockResourceFactory> mock_resource_factory;
    std::shared_ptr<mtd::MockDisplaySupportProvider> mock_fb_device;
    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
};
}

TEST_F(AndroidDisplayFactoryTest, hwc_selection_gets_fb_devices_ok)
{
    using namespace testing;

    EXPECT_CALL(*mock_resource_factory, create_fb_device())
        .Times(1);
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1);
    mga::AndroidDisplayFactory display_factory(mock_resource_factory, mock_display_report); 
}

/* this case occurs when the system cannot find the hwc library. it is a nonfatal error because we have a backup to try */
TEST_F(AndroidDisplayFactoryTest, hwc_module_unavailble_always_creates_gpu_display)
{
    using namespace testing;
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_CALL(*mock_resource_factory, create_fb_device())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_display(_,_))
        .Times(1);

    mga::AndroidDisplayFactory display_factory(mock_resource_factory, mock_display_report); 
    display_factory.create_display();
}

/* this case occurs when the hwc library doesn't work (error) */
TEST_F(AndroidDisplayFactoryTest, hwc_module_unopenable_uses_gpu)
{
    using namespace testing;
    EXPECT_CALL(*mock_resource_factory, create_hwc_1_1(_,_))
        .Times(0);
    
    mtd::FailingHardwareModuleStub failing_hwc_module_stub;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(&failing_hwc_module_stub), Return(0)));
    EXPECT_CALL(*mock_resource_factory, create_fb_device())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);

    mga::AndroidDisplayFactory display_factory(mock_resource_factory, mock_display_report); 
    display_factory.create_display();
}

TEST_F(AndroidDisplayFactoryTest, hwc_with_hwc_device_version_10_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;

    std::shared_ptr<mga::DisplaySupportProvider> mock_hwc_device;
  
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_hwc_1_0(_,_))
        .Times(1)
        .WillOnce(Return(mock_hwc_device));
    EXPECT_CALL(*mock_resource_factory, create_fb_device())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,0))
        .Times(1);

    std::shared_ptr<mg::DisplayReport> tmp2 = mock_display_report;

    mga::AndroidDisplayFactory display_factory(mock_resource_factory, mock_display_report);
    display_factory.create_display();
}

TEST_F(AndroidDisplayFactoryTest, hwc_with_hwc_device_version_11_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    std::shared_ptr<mga::DisplaySupportProvider> mock_hwc_device;
  
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_hwc_1_1(_,_))
        .Times(1)
        .WillOnce(Return(mock_hwc_device));
    EXPECT_CALL(*mock_resource_factory, create_fb_device())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,1))
        .Times(1);

    mga::AndroidDisplayFactory display_factory(mock_resource_factory, mock_display_report);
    display_factory.create_display();
}

// TODO: kdub support v1.2. for the time being, alloc a fallback gpu display
TEST_F(AndroidDisplayFactoryTest, hwc_with_hwc_device_failure_because_hwc_version12_not_supported)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_2;

    EXPECT_CALL(*mock_resource_factory, create_fb_device())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_display(_,_))
        .Times(1);

    mga::AndroidDisplayFactory display_factory(mock_resource_factory, mock_display_report); 
    display_factory.create_display();
}
