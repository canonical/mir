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
        ON_CALL(*this, create_hwc1_1_gpu_display())
            .WillByDefault(Return(std::make_shared<mtd::NullDisplay>()));
        ON_CALL(*this, create_gpu_display())
            .WillByDefault(Return(std::make_shared<mtd::NullDisplay>()));
    }
    MOCK_CONST_METHOD0(create_hwc1_1_gpu_display, std::shared_ptr<mg::Display>());
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

TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_success)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(_, _))
        .Times(1);
    EXPECT_CALL(*mock_fb_factory, create_hwc1_1_gpu_display())
        .Times(1);

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}

TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_failure_because_module_not_found)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(_, _))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(*mock_fb_factory, create_gpu_display())
        .Times(1);

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}

TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_failure_because_hwc_does_not_open)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), Return(-1)));
;
    EXPECT_CALL(*mock_fb_factory, create_gpu_display())
        .Times(1);

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}

TEST_F(AndroidFramebufferSelectorTest, hwc_with_hwc_device_failure_because_hwc_version_not_supported)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;
    EXPECT_CALL(hw_access_mock, hw_get_module(_, _))
        .Times(1);
    EXPECT_CALL(*mock_fb_factory, create_gpu_display())
        .Times(1);

    mga::AndroidDisplaySelector selector(mock_fb_factory);
    selector.primary_display();
}
