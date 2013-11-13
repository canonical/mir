/*
 * Copyright © 2012 Canonical Ltd.
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

#include "src/server/graphics/android/display_buffer_factory.h"
#include "src/server/graphics/android/resource_factory.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_buffer_packer.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/stub_display_device.h"
#include <system/window.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{
struct MockResourceFactory: public mga::DisplayResourceFactory
{
    ~MockResourceFactory() noexcept {}
    MockResourceFactory()
    {
        using namespace testing;
        ON_CALL(*this, create_hwc_native_device()).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_fb_native_device()).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_native_window(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_fb_device(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_hwc11_device(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_hwc10_device(_,_)).WillByDefault(Return(nullptr));
    }

    MOCK_CONST_METHOD0(create_hwc_native_device, std::shared_ptr<hwc_composer_device_1>());
    MOCK_CONST_METHOD0(create_fb_native_device, std::shared_ptr<framebuffer_device_t>());

    MOCK_CONST_METHOD1(create_native_window,
        std::shared_ptr<ANativeWindow>(std::shared_ptr<mga::DisplayDevice> const&));

    MOCK_CONST_METHOD1(create_fb_device,
        std::shared_ptr<mga::DisplayDevice>(std::shared_ptr<framebuffer_device_t> const&));
    MOCK_CONST_METHOD1(create_hwc11_device,
        std::shared_ptr<mga::DisplayDevice>(std::shared_ptr<hwc_composer_device_1> const&));
    MOCK_CONST_METHOD2(create_hwc10_device,
        std::shared_ptr<mga::DisplayDevice>(
            std::shared_ptr<hwc_composer_device_1> const&, std::shared_ptr<framebuffer_device_t> const&));
};

class DisplayBufferCreation : public ::testing::Test
{
public:
    void SetUp()
    {
        using namespace testing;
        mock_resource_factory = std::make_shared<testing::NiceMock<MockResourceFactory>>();
        ON_CALL(*mock_resource_factory, create_hwc_native_device())
            .WillByDefault(Return(hw_access_mock.mock_hwc_device)); 
        mock_display_report = std::make_shared<mtd::MockDisplayReport>();
    }

    mtd::MockEGL mock_egl;
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
    std::shared_ptr<MockResourceFactory> mock_resource_factory;
    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
};
}

TEST_F(DisplayBufferCreation, hwc_version_10_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_hwc10_device(_,_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,0))
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report, false);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, hwc_version_10_failure_uses_gpu)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report, false);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, hwc_version_11_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_hwc11_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,1))
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report, false);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, hwc_version_11_hwc_failure)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report, false);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, hwc_version_11_hwc_and_fb_failure_fatal)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THROW({
        mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report, false);
    }, std::runtime_error);
}

//we don't support hwc 1.2 quite yet. for the time being, at least try the fb backup
TEST_F(DisplayBufferCreation, hwc_version_12_attempts_fb_backup)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_2;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report, false);
    factory.create_display_device();
}

#if 0
TEST_F(DisplayBufferCreation, db_creation)
{
    using namespace testing;
    mtd::StubDisplayDevice stub_device;
    EGLDisplay disp = reinterpret_cast<EGLDisplay>(0x3);
    EGLConfig conf = reinterpret_cast<EGLConfig>(0x3);
    EGLContext cont = reinterpret_cast<EGLContext>(0x3);
    EXPECT_CALL(*mock_resource_factory, create_native_window(_))
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report, false);
    factory.create_display_buffer(mt::fake_shared(stub_device), disp, conf, cont);
}
#endif
