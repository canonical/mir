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
#include "mir_test_doubles/mock_egl.h"

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
        mock_display_info_provider = std::make_shared<NiceMock<mtd::MockDisplaySupportProvider>>();
        mock_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();
        fake_fb_num = 2;

        ON_CALL(*mock_display_info_provider, display_format())
            .WillByDefault(Return(geom::PixelFormat::argb_8888));
        ON_CALL(*mock_display_info_provider, display_size())
            .WillByDefault(Return(geom::Size{2, 3}));
        ON_CALL(*mock_display_info_provider, number_of_framebuffers_available())
            .WillByDefault(Return(fake_fb_num));
        ON_CALL(*mock_buffer_allocator, alloc_buffer_platform(_,_,_))
            .WillByDefault(Return(std::shared_ptr<mga::Buffer>()));
    }

    std::shared_ptr<mtd::MockDisplayReport> mock_report;
    std::shared_ptr<MockAndroidGraphicBufferAllocator> mock_buffer_allocator;
    std::shared_ptr<mtd::MockDisplaySupportProvider> mock_display_info_provider;
    unsigned int fake_fb_num;
    mtd::HardwareAccessMock hw_access_mock;
    testing::NiceMock<mtd::MockEGL> mock_egl;
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

TEST_F(FBFactory, test_native_window_creation_uses_size)
{
    using namespace testing;

    mga::ResourceFactory factory(mock_buffer_allocator);

    geom::Width disp_width{44};
    geom::Height disp_height{4567654};
    geom::Size disp_size{disp_width, disp_height};   
 
    EXPECT_CALL(*mock_display_info_provider, display_size())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(disp_size));
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

TEST_F(FBFactory, test_native_window_creation_uses_argb_8888)
{
    using namespace testing;

    mga::ResourceFactory factory(mock_buffer_allocator);
    geom::PixelFormat pf = geom::PixelFormat::argb_8888; 
 
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
