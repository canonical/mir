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

#include "src/platforms/android/server/resource_factory.h"
#include "src/platforms/android/server/hwc_loggers.h"
#include "mir/test/doubles/mock_android_hw.h"

#include <stdexcept>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

struct ResourceFactoryTest  : public ::testing::Test
{
    mtd::HardwareAccessMock hw_access_mock;
    std::shared_ptr<mga::HwcReport> null_report{std::make_shared<mga::NullHwcReport>()};
};

TEST_F(ResourceFactoryTest, fb_native_creation_opens_and_closes_gralloc)
{
    using namespace testing;
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::ResourceFactory factory;
    factory.create_fb_native_device();
    EXPECT_TRUE(hw_access_mock.open_count_matches_close());
}

TEST_F(ResourceFactoryTest, test_device_creation_throws_on_failure)
{
    using namespace testing;
    mga::ResourceFactory factory;

    /* failure because of rc */
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        factory.create_fb_native_device();
    }, std::runtime_error);

    /* failure because of nullptr returned */
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(nullptr),Return(-1)));

    EXPECT_THROW({
        factory.create_fb_native_device();
    }, std::runtime_error);
}

TEST_F(ResourceFactoryTest, hwc_allocation)
{
    using namespace testing;
    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_2;
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::ResourceFactory factory;
    auto wrapper_tuple = factory.create_hwc_wrapper(null_report);
    EXPECT_THAT(std::get<1>(wrapper_tuple), Eq(mga::HwcVersion::hwc12));
    std::get<0>(wrapper_tuple).reset();

    EXPECT_TRUE(hw_access_mock.open_count_matches_close());
}

TEST_F(ResourceFactoryTest, hwc_allocation_failures)
{
    using namespace testing;

    mtd::FailingHardwareModuleStub failing_hwc_module_stub;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(2)
        .WillOnce(Return(-1))
        .WillOnce(DoAll(SetArgPointee<1>(&failing_hwc_module_stub), Return(0)));

    mga::ResourceFactory factory;

    EXPECT_THROW({
        factory.create_hwc_wrapper(null_report);
    }, std::runtime_error);
    EXPECT_THROW({
        factory.create_hwc_wrapper(null_report);
    }, std::runtime_error);

    EXPECT_TRUE(hw_access_mock.open_count_matches_close());
}
