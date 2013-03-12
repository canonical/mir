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

#include "src/graphics/android/android_fb_factory.h"
#include "src/graphics/android/android_display_selector.h"

#include "mir_test/hw_mock.h"
#include <hardware/hwcomposer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt=mir::test;
namespace mga=mir::graphics::android;

class MockFBFactory : public mga::AndroidFBFactory
{
    MOCK_CONST_METHOD0(create_hwc1_1_gpu_display, std::shared_ptr<mga::AndroidDisplay>());
    MOCK_CONST_METHOD0(create_gpu_display, std::shared_ptr<mga::AndroidDisplay>());
};

class DisplayMethodSelectorTest : public ::testing::Test
{
public:
    DisplayMethodSelectorTest()
    {
    }

    mt::HardwareAccessMock hw_access_mock;
};

TEST_F(DisplayMethodSelectorTest, hwc_selection_gets_hwc_device)
{
    using namespace testing;
    mga::AndroidDisplaySelector selector;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1);

    selector->primary_display();
}

#if 0
TEST_F(DisplayMethodSelectorTest, hwc_with_hwc_device_success)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(_, _))
        .Times(1);
    EXPECT_CALL(*mock_fb_factory, create_hwc_gpu_display)
        .Times(1); 

    selector->primary_display();
}

TEST_F(DisplayMethodSelectorTest, hwc_with_hwc_device_failure_because_module_not_found)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);
    EXPECT_CALL(*mock_fb_factory, create_gpu_display)
        .Times(1); 

    auto display = selector->primary_display();
}

TEST_F(DisplayMethodSelectorTest, hwc_with_hwc_device_failure_because_hwc_version_not_supported)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);
    EXPECT_CALL(*mock_fb_factory, create_gpu_display)
        .Times(1); 

    auto display = selector->primary_display();
}

TEST_F(DisplayMethodSelectorTest, double_primary_display_call_returns_same_object)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(_, _))
        .Times(2);

    EXPECT_CALL(*mock_fb_factory, create_hwc_gpu_display)
        .Times(1); 

    auto display = selector->primary_display();
    auto display2 = selector->primary_display();

    EXPECT_EQ(display, display2);
}
#endif
