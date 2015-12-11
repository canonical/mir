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

#include "src/platforms/android/server/android_alloc_adaptor.h"
#include "src/platforms/android/server/device_quirks.h"
#include "mir/graphics/android/native_buffer.h"

#include "mir/test/doubles/mock_android_alloc_device.h"
#include "mir/test/doubles/mock_alloc_adaptor.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;


class AdaptorICSTest : public ::testing::Test
{
public:
    AdaptorICSTest()
     : fb_usage_flags(GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_FB),
       hw_usage_flags(GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER),
       sw_usage_flags(GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_TEXTURE)
    {}

    virtual void SetUp()
    {
        using namespace testing;
        mock_alloc_device = std::make_shared<NiceMock<mtd::MockAllocDevice>>();

        auto quirks = std::make_shared<mga::DeviceQuirks>(mga::PropertiesOps{});
        alloc_adaptor = std::make_shared<mga::AndroidAllocAdaptor>(mock_alloc_device, quirks);

        pf = mir_pixel_format_abgr_8888;
        size = geom::Size{300, 200};
        usage = mga::BufferUsage::use_hardware;
    }

    std::shared_ptr<mtd::MockAllocDevice> mock_alloc_device;
    std::shared_ptr<mga::AndroidAllocAdaptor> alloc_adaptor;

    MirPixelFormat pf;
    geom::Size size;
    mga::BufferUsage usage;
    int const fb_usage_flags;
    int const hw_usage_flags;
    int const sw_usage_flags;
};

TEST_F(AdaptorICSTest, resource_type_test_fail_ret)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(mock_alloc_device->buffer_handle),
                  SetArgPointee<6>(size.width.as_uint32_t()*4),
                  Return(-1)));

    EXPECT_THROW({
        alloc_adaptor->alloc_buffer(size, pf, usage);
    }, std::runtime_error);
}

TEST_F(AdaptorICSTest, resource_type_test_fail_stride)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(mock_alloc_device->buffer_handle),
                  SetArgPointee<6>(0),
                  Return(0)));

    EXPECT_THROW({
        alloc_adaptor->alloc_buffer(size, pf, usage);
    }, std::runtime_error);
}

TEST_F(AdaptorICSTest, resource_type_test_fail_null_handle)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(nullptr),
                  SetArgPointee<6>(size.width.as_uint32_t()*4),
                  Return(0)));

    EXPECT_THROW({
        alloc_adaptor->alloc_buffer(size, pf, usage);
    }, std::runtime_error);
}

TEST_F(AdaptorICSTest, resource_type_test_proper_alloc_is_used)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface(mock_alloc_device.get(),_,_,_,_,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(mock_alloc_device.get(),_));

    alloc_adaptor->alloc_buffer(size, pf, usage);
}

TEST_F(AdaptorICSTest, resource_type_test_deleter_deletes_correct_handle)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(mock_alloc_device->buffer_handle),
                  SetArgPointee<6>(size.width.as_uint32_t()*4),
                  Return(0)));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,mock_alloc_device->buffer_handle));

    alloc_adaptor->alloc_buffer(size, pf, usage);
}

TEST_F(AdaptorICSTest, adaptor_gralloc_format_conversion_abgr8888)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,HAL_PIXEL_FORMAT_RGBA_8888,_,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_));

    alloc_adaptor->alloc_buffer(size, pf, usage);
}

TEST_F(AdaptorICSTest, adaptor_gralloc_dimension_conversion)
{
    using namespace testing;
    int w = size.width.as_uint32_t();
    int h = size.height.as_uint32_t();
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,w,h,_,_,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_));

    alloc_adaptor->alloc_buffer(size, pf, usage );
}

TEST_F(AdaptorICSTest, adaptor_gralloc_hw_usage_conversion)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,hw_usage_flags,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_) );

    alloc_adaptor->alloc_buffer(size, pf, usage );
}

TEST_F(AdaptorICSTest, adaptor_gralloc_sw_usage_conversion)
{
    using namespace testing;
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,sw_usage_flags,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_));

    alloc_adaptor->alloc_buffer(size, pf, mga::BufferUsage::use_software);
}

TEST_F(AdaptorICSTest, adaptor_gralloc_usage_conversion_fb_gles)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,fb_usage_flags,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_));

    alloc_adaptor->alloc_buffer(size, pf, mga::BufferUsage::use_framebuffer_gles);
}

TEST_F(AdaptorICSTest, handle_size_is_correct)
{
    auto native_handle = alloc_adaptor->alloc_buffer(size, pf, usage);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(static_cast<int>(size.width.as_uint32_t()), anwb->width);
    EXPECT_EQ(static_cast<int>(size.height.as_uint32_t()), anwb->height);
    EXPECT_EQ(static_cast<int>(mock_alloc_device->fake_stride), anwb->stride);
}

TEST_F(AdaptorICSTest, handle_buffer_pf_is_converted_to_android_abgr_8888)
{
    auto native_handle = alloc_adaptor->alloc_buffer(size, pf, usage);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(HAL_PIXEL_FORMAT_RGBA_8888, anwb->format);
}

TEST_F(AdaptorICSTest, handle_buffer_pf_is_converted_to_android_xbgr_8888)
{
    auto native_handle = alloc_adaptor->alloc_buffer(size, mir_pixel_format_xbgr_8888, usage);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(HAL_PIXEL_FORMAT_RGBX_8888, anwb->format);
}

TEST_F(AdaptorICSTest, handle_buffer_usage_is_converted_to_android_use_hw)
{
    auto native_handle = alloc_adaptor->alloc_buffer(size, pf, usage);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(hw_usage_flags, anwb->usage);
}

TEST_F(AdaptorICSTest, handle_buffer_usage_is_converted_to_android_use_fb)
{
    auto native_handle = alloc_adaptor->alloc_buffer(size, pf, mga::BufferUsage::use_framebuffer_gles);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(fb_usage_flags, anwb->usage);
}

TEST_F(AdaptorICSTest, handle_has_strong_reference_for_c_drivers)
{
    auto native_handle = alloc_adaptor->alloc_buffer(size, pf, usage);
    auto anwb = native_handle->anwb();
    ASSERT_NE(nullptr, anwb->common.incRef);
    ASSERT_NE(nullptr, anwb->common.decRef);
    anwb->common.incRef(&anwb->common);
    anwb->common.decRef(&anwb->common);
}

TEST_F(AdaptorICSTest, handle_has_right_magic)
{
    int magic = ANDROID_NATIVE_MAKE_CONSTANT('_','b','f','r');  /* magic value shared by JB and ICS */
    auto native_handle = alloc_adaptor->alloc_buffer(size, pf, usage);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(magic, anwb->common.magic);
}

TEST_F(AdaptorICSTest, handle_has_version)
{
    int version = sizeof(ANativeWindowBuffer);  /* version value shared by JB and ICS */
    auto native_handle = alloc_adaptor->alloc_buffer(size, pf, usage);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(version, anwb->common.version);
}
