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

#include "src/server/graphics/android/resource_factory.h"
#include "src/server/graphics/android/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir_test_doubles/mock_display_support_provider.h"
#include "mir_test_doubles/mock_display_report.h"

#include "mir_test_doubles/mock_android_hw.h"

#include <stdexcept>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace mt=mir::test;
namespace geom=mir::geometry;

namespace
{

class MockAndroidGraphicBufferAllocator : public mga::GraphicBufferAllocator
{
public:
    MOCK_METHOD1(alloc_buffer, std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
    MOCK_METHOD3(alloc_buffer_platform, std::shared_ptr<mga::Buffer>(geom::Size, geom::PixelFormat, mga::BufferUsage));
    MOCK_METHOD0(supported_pixel_formats, std::vector<geom::PixelFormat>());

    ~MockAndroidGraphicBufferAllocator() noexcept {}
};
}

class FBFactory  : public ::testing::Test
{
public:
    void SetUp()
    {
        using namespace testing;
        mock_buffer_allocator = std::make_shared<NiceMock<MockAndroidGraphicBufferAllocator>>();
        mock_display_info = std::make_shared<NiceMock<mtd::MockDisplayInfo>>();
        mock_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();
        fake_fb_num = 2;

        ON_CALL(*mock_display_info_provider, display_format())
            .WillByDefault(Return(geom::PixelFormat::abgr_8888));
        ON_CALL(*mock_display_info_provider, display_size())
            .WillByDefault(Return(geom::Size{2, 3}));
        ON_CALL(*mock_display_info_provider, number_of_framebuffers_available())
            .WillByDefault(Return(fake_fb_num));

        ON_CALL(*mock_buffer_allocator, alloc_buffer_platform(_,_,_))
            .WillByDefault(Return(std::shared_ptr<mga::Buffer>()));
    }

    std::shared_ptr<mtd::MockDisplayReport> mock_report;
    std::shared_ptr<MockAndroidGraphicBufferAllocator> mock_buffer_allocator;
    std::shared_ptr<mtd::MockDisplayInfo> mock_display_info_provider;
    unsigned int fake_fb_num;
    mtd::HardwareAccessMock hw_access_mock;
};

TEST_F(FBFactory, test_native_window_creation_figures_out_fb_number)
{
    using namespace testing;

    mga::ResourceFactory factory(mock_buffer_allocator);
 
    EXPECT_CALL(*mock_display_info_provider, number_of_framebuffers_available())
        .Times(1);
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer_platform(_,_,_))
        .Times(fake_fb_num);
 
    factory.create_display(mock_display_info_provider, mock_report);
}

#if 0
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
        mock_resource_factory = std::make_shared<testing::StrictMock<MockResourceFactory>>();
        mock_fb_device = std::make_shared<testing::NiceMock<mtd::MockDisplaySupportProvider>>();
        mock_display_report = std::make_shared<testing::NiceMock<mtd::MockDisplayReport>>();

        ON_CALL(*mock_resource_factory, create_fb_device())
            .WillByDefault(Return(mock_fb_device));
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

    std::shared_ptr<mg::DisplayReport> tmp2 = mock_display_report;

    mga::AndroidDisplayFactory display_factory(mock_resource_factory, mock_display_report);
    display_factory.create_display();
}

#endif 



#if 0
TEST_F(AndroidDisplayFactoryTest, hwc_with_hwc_device_version_11_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    std::shared_ptr<mga::HWCDevice> mock_hwc_device;
  
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .Times(1);
    EXPECT_CALL(*mock_hwc_factory, create_hwc_1_1(_,_))
        .Times(1)
        .WillOnce(Return(mock_hwc_device));
    EXPECT_CALL(*mock_fnw_factory, create_fb_device())
        .Times(1);

    std::shared_ptr<mga::DisplaySupportProvider> tmp = mock_hwc_device;
    EXPECT_CALL(*mock_fnw_factory, create_fb_native_window(tmp))
        .Times(1);
    std::shared_ptr<mg::DisplayReport> tmp2 = mock_display_report;
    EXPECT_CALL(*mock_display_allocator, create_display(mock_hwc_device, stub_anativewindow, tmp2))
        .Times(1);

    mga::AndroidDisplayFactory display_factory(mock_display_allocator, mock_hwc_factory,
                                               mock_fnw_factory, mock_display_report); 
    display_factory.create_display();
}

TEST_F(AndroidDisplayFactoryTest, gpu_logging)
{
    using namespace testing;
    mtd::FailingHardwareModuleStub failing_hwc_module_stub;
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(&failing_hwc_module_stub), Return(0)));

    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);
    mga::AndroidDisplayFactory display_factory(mock_display_allocator, mock_hwc_factory,
                                               mock_fnw_factory, mock_display_report); 
    display_factory.create_display();
}

TEST_F(AndroidDisplayFactoryTest, hwc10_logging)
{
    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;

    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,0))
        .Times(1);
    mga::AndroidDisplayFactory display_factory(mock_display_allocator, mock_hwc_factory,
                                               mock_fnw_factory, mock_display_report); 
    display_factory.create_display();
}

TEST_F(AndroidDisplayFactoryTest, hwc11_logging)
{
    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,1))
        .Times(1);
    mga::AndroidDisplayFactory display_factory(mock_display_allocator, mock_hwc_factory,
                                               mock_fnw_factory, mock_display_report); 
    display_factory.create_display();
}

// TODO: kdub support v1.2. for the time being, alloc a fallback gpu display
TEST_F(AndroidDisplayFactoryTest, hwc_with_hwc_device_failure_because_hwc_version12_not_supported)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_2;

    EXPECT_CALL(*mock_fnw_factory, create_fb_device())
        .Times(1);
    std::shared_ptr<mga::DisplaySupportProvider> tmp = mock_fb_device;
    EXPECT_CALL(*mock_fnw_factory, create_fb_native_window(tmp))
        .Times(1);
    EXPECT_CALL(*mock_hwc_factory, create_hwc_1_1(_,_))
        .Times(0);
    EXPECT_CALL(*mock_display_allocator, create_display(_,_,_))
        .Times(0);
    std::shared_ptr<mg::DisplayReport> tmp2 = mock_display_report;
    EXPECT_CALL(*mock_display_allocator, create_display(_,_,tmp2))
        .Times(1);

    mga::AndroidDisplayFactory display_factory(mock_display_allocator, mock_hwc_factory, mock_fnw_factory, mock_display_report); 
    display_factory.create_display();
}
#endif
#if 0
TEST_F(FBFactory, test_native_window_creation_uses_size)
{
    using namespace testing;

    mga::ResourceFactory factory(mock_buffer_allocator);

    geom::Width disp_width{44};
    geom::Height disp_height{4567654};
    geom::Size disp_size{disp_width, disp_height};   
 
    EXPECT_CALL(*mock_display_info_provider, display_size())
        .Times(1)
        .WillOnce(Return(disp_size));
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer_platform(disp_size,_,_))
        .Times(fake_fb_num);
 
    factory.create_display(mock_display_info_provider, mock_report);
} 

TEST_F(FBFactory, test_native_window_creation_specifies_buffer_type)
{
    using namespace testing;

    mga::ResourceFactory factory(mock_buffer_allocator);

    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer_platform(_,_,mga::BufferUsage::use_framebuffer_gles))
        .Times(fake_fb_num);
 
    factory.create_display(mock_display_info_provider, mock_report);
} 

//note: @kdub imo, the hwc api has a hole in it that it doesn't allow query for format. surfaceflinger code
//            makes note of this api hole in its comments too. It always uses rgba8888, which we will do too.
TEST_F(FBFactory, test_native_window_creation_uses_rgba8888)
{
    using namespace testing;

    mga::ResourceFactory factory(mock_buffer_allocator);
    geom::PixelFormat pf = geom::PixelFormat::abgr_8888; 
 
    EXPECT_CALL(*mock_display_info_provider, display_format())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(pf));
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer_platform(_,pf,_))
        .Times(fake_fb_num);
 
    factory.create_display(mock_display_info_provider, mock_report);
}

TEST_F(FBFactory, test_device_creation_accesses_gralloc)
{
    using namespace testing;
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::ResourceFactory factory(mock_buffer_allocator);
    factory.create_fb_device();
}

TEST_F(FBFactory, test_device_creation_throws_on_failure)
{
    using namespace testing;
    mga::ResourceFactory factory(mock_buffer_allocator);

    /* failure because of rc */
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        factory.create_fb_device();
    }, std::runtime_error);

    /* failure because of nullptr returned */
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(nullptr),Return(-1)));

    EXPECT_THROW({
        factory.create_fb_device();
    }, std::runtime_error);

}

TEST_F(FBFactory, test_device_creation_resource_has_fb_close_on_destruct)
{
    using namespace testing;
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::ResourceFactory factory(mock_buffer_allocator);
    factory.create_fb_device();

    EXPECT_TRUE(hw_access_mock.open_count_matches_close());
}
#endif
