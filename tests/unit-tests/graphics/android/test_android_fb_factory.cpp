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

#include "mir_test_doubles/mock_android_hwc_device.h"

namespace mtd=mir::test::doubles;

/*
HWComposerAdaptor
{

};
*/
class AndroidFBFactory
{
    //virtual dest

    //creates a display that will render primarily via the gpu and OpenGLES 2.0, but will use the hwc
    //module (version 1.1) for additional functionality, such as vsync timings, and hotplug detection 
    virtual std::shared_ptr<mg::AndroidDisplay> create_hwc1_1_gpu_display() const = 0;

    //creates a display that will render primarily via the gpu and OpenGLES 2.0. Primarily a fall-back mode, this display is similar to what Android does when /system/lib/hw/hwcomposer.*.so modules are not present 
    virtual std::shared_ptr<mg::AndroidDisplay> create_gpu_display() const = 0;
};
 
class MockFBFactory : public mga::AndroidFBFactory
{
    MOCK_CONST_METHOD0(create_hwc1_1_gpu_display, std::shared_ptr<mg::AndroidDisplay>());
    MOCK_CONST_METHOD0(create_gpu_display, std::shared_ptr<mg::AndroidDisplay>());
};

class DisplayMethodSelector : public ::testing::Test
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

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);

    selector->primary_display();
}

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

}

