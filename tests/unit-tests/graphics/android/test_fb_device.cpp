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

#include "mir_test_doubles/mock_fb_hal_device.h"
#include "mir_test_doubles/mock_android_buffer.h"
#include "src/server/graphics/android/fb_device.h"
#include "mir_test/hw_mock.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mt=mir::test;

struct FBDevice : public ::testing::Test
{
    virtual void SetUp()
    {
        using namespace testing;
        
        width = 413;
        height = 516;
        fbnum = 4;
        format = HAL_PIXEL_FORMAT_RGBA_8888;

        fb_hal_mock = std::make_shared<mtd::MockFBHalDevice>(width, height, format, fbnum); 
        mock_buffer = std::make_shared<NiceMock<mtd::MockAndroidBuffer>>();

        dummy_buffer = std::make_shared<ANativeWindowBuffer>();
        dummy_buffer->handle = (buffer_handle_t) 0x4893;
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(dummy_buffer));
    }

    unsigned int width, height, format, fbnum;
    std::shared_ptr<mtd::MockFBHalDevice> fb_hal_mock;
    std::shared_ptr<mtd::MockAndroidBuffer> mock_buffer;
    std::shared_ptr<ANativeWindowBuffer> dummy_buffer;
    mt::HardwareAccessMock hw_access_mock;
};

TEST_F(FBDevice, set_next_frontbuffer_ok)
{
    using namespace testing;
    mga::FBDevice fbdev(fb_hal_mock);

    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(),dummy_buffer->handle))
        .Times(1);

    fbdev.set_next_frontbuffer(mock_buffer); 
}

TEST_F(FBDevice, set_next_frontbuffer_fail)
{
    using namespace testing;
    mga::FBDevice fbdev(fb_hal_mock);

    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(),dummy_buffer->handle))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        fbdev.set_next_frontbuffer(mock_buffer); 
    }, std::runtime_error); 
}

TEST_F(FBDevice, determine_size)
{
    mga::FBDevice fbdev(fb_hal_mock);
    auto size = fbdev.display_size();
    EXPECT_EQ(width, size.width.as_uint32_t());
    EXPECT_EQ(height, size.height.as_uint32_t());
}

TEST_F(FBDevice, determine_fbnum)
{
    mga::FBDevice fbdev(fb_hal_mock);
    EXPECT_EQ(fbnum, fbdev.number_of_framebuffers_available());
}

//some drivers incorrectly report 0 buffers available. if this is true, we should alloc 2, the minimum requirement
TEST_F(FBDevice, determine_fbnum_always_reports_2_minimum)
{
    auto slightly_malformed_fb_hal_mock = std::make_shared<mtd::MockFBHalDevice>(width, height, format, 0); 
    mga::FBDevice fbdev(slightly_malformed_fb_hal_mock);
    EXPECT_EQ(2u, fbdev.number_of_framebuffers_available());
}

TEST_F(FBDevice, determine_pixformat)
{
    mga::FBDevice fbdev(fb_hal_mock);
    EXPECT_EQ(geom::PixelFormat::abgr_8888, fbdev.display_format());
}
