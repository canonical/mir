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

#include "src/platforms/android/server/gralloc_module.h"
#include "src/platforms/android/server/device_quirks.h"
#include "src/platforms/android/server/cmdstream_sync_factory.h"
#include "mir/options/program_option.h"
#include "native_buffer.h"

#include "mir/test/doubles/mock_android_alloc_device.h"
#include "mir/test/doubles/mock_egl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
using namespace testing;

struct Gralloc : Test
{
    Gralloc():
        mock_alloc_device(std::make_shared<NiceMock<mtd::MockAllocDevice>>()),
        gralloc(std::make_shared<mga::GrallocModule>(
            mock_alloc_device, sync_factory, std::make_shared<mga::DeviceQuirks>(mga::PropertiesOps{}))),
        pf(mir_pixel_format_abgr_8888),
        size{300, 200}
    {
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mga::CommandStreamSyncFactory> sync_factory{std::make_shared<mga::EGLSyncFactory>()};
    std::shared_ptr<mtd::MockAllocDevice> mock_alloc_device;
    std::shared_ptr<mga::GrallocModule> gralloc;

    MirPixelFormat pf;
    uint32_t android_pf = HAL_PIXEL_FORMAT_RGBA_8888;
    geom::Size size;
    mg::BufferUsage usage = mg::BufferUsage::hardware;
    int const hw_usage_flags
       {GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER};
};

TEST_F(Gralloc, resource_type_test_fail_ret)
{
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(mock_alloc_device->buffer_handle),
                  SetArgPointee<6>(size.width.as_uint32_t()*4),
                  Return(-1)));

    EXPECT_THROW({
        gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    }, std::runtime_error);
}

TEST_F(Gralloc, resource_type_test_fail_stride)
{
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(mock_alloc_device->buffer_handle),
                  SetArgPointee<6>(0),
                  Return(0)));

    EXPECT_THROW({
        gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    }, std::runtime_error);
}

TEST_F(Gralloc, resource_type_test_fail_null_handle)
{
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(nullptr),
                  SetArgPointee<6>(size.width.as_uint32_t()*4),
                  Return(0)));

    EXPECT_THROW({
        gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    }, std::runtime_error);
}

TEST_F(Gralloc, resource_type_test_proper_alloc_is_used)
{
    EXPECT_CALL(*mock_alloc_device, alloc_interface(mock_alloc_device.get(),_,_,_,_,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(mock_alloc_device.get(),_));

    gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
}

TEST_F(Gralloc, resource_type_test_deleter_deletes_correct_handle)
{
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,_,_,_))
    .WillOnce(DoAll(
                  SetArgPointee<5>(mock_alloc_device->buffer_handle),
                  SetArgPointee<6>(size.width.as_uint32_t()*4),
                  Return(0)));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,mock_alloc_device->buffer_handle));

    gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
}

TEST_F(Gralloc, adaptor_gralloc_format_conversion_abgr8888)
{
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,HAL_PIXEL_FORMAT_RGBA_8888,_,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_));

    gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
}

TEST_F(Gralloc, adaptor_gralloc_dimension_conversion)
{
    int w = size.width.as_uint32_t();
    int h = size.height.as_uint32_t();
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,w,h,_,_,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_));

    gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
}

TEST_F(Gralloc, adaptor_gralloc_usage_correct)
{
    EXPECT_CALL(*mock_alloc_device, alloc_interface(_,_,_,_,hw_usage_flags,_,_));
    EXPECT_CALL(*mock_alloc_device, free_interface(_,_) );

    gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
}

TEST_F(Gralloc, handle_size_is_correct)
{
    auto native_handle = gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(static_cast<int>(size.width.as_uint32_t()), anwb->width);
    EXPECT_EQ(static_cast<int>(size.height.as_uint32_t()), anwb->height);
    EXPECT_EQ(static_cast<int>(mock_alloc_device->fake_stride), anwb->stride);
}

TEST_F(Gralloc, handle_buffer_pf_is_reflected_in_format_field)
{
    auto native_handle = gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(HAL_PIXEL_FORMAT_RGBA_8888, anwb->format);
}

TEST_F(Gralloc, handle_buffer_usage_is_reflected_in_usage)
{
    auto native_handle = gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(hw_usage_flags, anwb->usage);
}

TEST_F(Gralloc, handle_has_strong_reference_for_c_drivers)
{
    auto native_handle = gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    auto anwb = native_handle->anwb();
    ASSERT_NE(nullptr, anwb->common.incRef);
    ASSERT_NE(nullptr, anwb->common.decRef);
    anwb->common.incRef(&anwb->common);
    anwb->common.decRef(&anwb->common);
}

TEST_F(Gralloc, handle_has_right_magic)
{
    int magic = ANDROID_NATIVE_MAKE_CONSTANT('_','b','f','r');  /* magic value shared by JB and ICS */
    auto native_handle = gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(magic, anwb->common.magic);
}

TEST_F(Gralloc, handle_has_version)
{
    int version = sizeof(ANativeWindowBuffer);  /* version value shared by JB and ICS */
    auto native_handle = gralloc->alloc_buffer(size, android_pf, hw_usage_flags);
    auto anwb = native_handle->anwb();
    EXPECT_EQ(version, anwb->common.version);
}
