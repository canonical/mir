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

#include "mir/graphics/egl_extensions.h"
#include "src/platforms/android/server/buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_fence.h"
#include "mir/test/doubles/mock_android_hw.h"
#include "mir/test/doubles/mock_android_native_buffer.h"

#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

class AndroidBuffer : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        mock_native_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();
        pf = mir_pixel_format_abgr_8888;

        anwb = mock_native_buffer->anwb();
        anwb->width = 44;
        anwb->height = 45;
        anwb->stride = anwb->width * MIR_BYTES_PER_PIXEL(pf);
        anwb->format = HAL_PIXEL_FORMAT_RGBA_8888;

        default_use = mga::BufferUsage::use_hardware;
        size = geom::Size{anwb->width, anwb->height};
        extensions = std::make_shared<mg::EGLExtensions>();
    }

    ANativeWindowBuffer *anwb;
    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> mock_native_buffer;
    MirPixelFormat pf;
    geom::Size size;
    mga::BufferUsage default_use;
    std::shared_ptr<mg::EGLExtensions> extensions;
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
    mtd::MockGrallocModule gralloc;
};

TEST_F(AndroidBuffer, size_query_test)
{
    using namespace testing;

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);

    geom::Size expected_size{anwb->width, anwb->height};
    EXPECT_EQ(expected_size, buffer.size());
}

TEST_F(AndroidBuffer, format_query_test)
{
    using namespace testing;

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    EXPECT_EQ(mir_pixel_format_abgr_8888, buffer.pixel_format());
}

TEST_F(AndroidBuffer, returns_native_buffer_times_two)
{
    using namespace testing;
    int acquire_fake_fence_fd1 = 948;
    int acquire_fake_fence_fd2 = 954;

    EXPECT_CALL(*mock_native_buffer, update_usage(acquire_fake_fence_fd1, mga::BufferAccess::write))
        .Times(1);
    EXPECT_CALL(*mock_native_buffer, update_usage(acquire_fake_fence_fd2, mga::BufferAccess::read))
        .Times(1);

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    {
        auto native_resource = buffer.native_buffer_handle();
        EXPECT_EQ(mock_native_buffer, native_resource);
        native_resource->update_usage(acquire_fake_fence_fd1, mga::BufferAccess::write);
    }
    {
        auto native_resource = buffer.native_buffer_handle();
        EXPECT_EQ(mock_native_buffer, native_resource);
        native_resource->update_usage(acquire_fake_fence_fd2, mga::BufferAccess::read);
    }
}

TEST_F(AndroidBuffer, queries_native_window_for_stride)
{
    using namespace testing;

    geom::Stride expected_stride{anwb->stride * MIR_BYTES_PER_PIXEL(pf)};
    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    EXPECT_EQ(expected_stride, buffer.stride());
}

TEST_F(AndroidBuffer, write_detects_incorrect_size)
{
    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    unsigned char pix = {0x32};
    EXPECT_THROW({
        buffer.write(&pix, sizeof(pix));
    }, std::logic_error);
}

TEST_F(AndroidBuffer, write_throws_on_failed_mapping_indicated_by_rc_code)
{
    using namespace testing;
    EXPECT_CALL(gralloc, lock_interface(_, _, _, _, _, _, _, _))
        .WillOnce(Return(-1));

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    size_t sz = anwb->height * anwb->width * MIR_BYTES_PER_PIXEL(pf);
    auto const pixels = std::shared_ptr<unsigned char>(
        static_cast<unsigned char*>(::operator new(sizeof(unsigned char) *sz)));
    EXPECT_THROW({
        buffer.write(pixels.get(), sz);
    }, std::runtime_error);
}

TEST_F(AndroidBuffer, write_throws_on_failed_mapping_indicated_by_nullptr_return)
{
    using namespace testing;
    EXPECT_CALL(gralloc, lock_interface(_, _, _, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<7>(nullptr), Return(0)));

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    size_t sz = anwb->height * anwb->width * MIR_BYTES_PER_PIXEL(pf);
    auto const pixels = std::shared_ptr<unsigned char>(
        static_cast<unsigned char*>(::operator new(sizeof(unsigned char) *sz)));
    EXPECT_THROW({
        buffer.write(pixels.get(), sz);
    }, std::runtime_error);
}

TEST_F(AndroidBuffer, writes_pixels)
{
    using namespace testing;
    size_t strided_sz = anwb->height * anwb->stride * MIR_BYTES_PER_PIXEL(pf);
    size_t sz = anwb->height * anwb->width * MIR_BYTES_PER_PIXEL(pf);
    auto const mapped_pixels = std::shared_ptr<unsigned char>(
        static_cast<unsigned char*>(::operator new(sizeof(unsigned char) * strided_sz)));

    EXPECT_CALL(*mock_native_buffer, ensure_available_for(mga::BufferAccess::write));
    EXPECT_CALL(gralloc, lock_interface(
        &gralloc,_, GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, size.width.as_int(), size.height.as_int(), _))
        .WillOnce(DoAll(SetArgPointee<7>(mapped_pixels.get()), Return(0)));
    EXPECT_CALL(gralloc, unlock_interface(_,_));

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);

    auto const pixels = std::shared_ptr<unsigned char>(
        static_cast<unsigned char*>(::operator new(sizeof(unsigned char) *sz)));
    buffer.write(pixels.get(), sz);
    Mock::VerifyAndClearExpectations(&gralloc);
}

TEST_F(AndroidBuffer, read_throws_on_failed_mapping_indicated_by_rc_code)
{
    using namespace testing;
    EXPECT_CALL(gralloc, lock_interface(_, _, _, _, _, _, _, _))
        .WillOnce(Return(-1));

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    EXPECT_THROW({
        buffer.read([](unsigned char const*){});
    }, std::runtime_error);
}

TEST_F(AndroidBuffer, read_throws_on_failed_mapping_indicated_by_nullptr_return)
{
    using namespace testing;
    EXPECT_CALL(gralloc, lock_interface(_, _, _, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<7>(nullptr), Return(0)));

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    EXPECT_THROW({
        buffer.read([](unsigned char const*){});
    }, std::runtime_error);
}

TEST_F(AndroidBuffer, reads_pixels)
{
    using namespace testing;
    size_t strided_sz = anwb->height * anwb->stride * MIR_BYTES_PER_PIXEL(pf);
    auto const mapped_pixels = std::shared_ptr<unsigned char>(
        static_cast<unsigned char*>(::operator new(sizeof(unsigned char) * strided_sz)));

    EXPECT_CALL(*mock_native_buffer, ensure_available_for(mga::BufferAccess::read));
    EXPECT_CALL(gralloc, lock_interface(
        &gralloc,_, GRALLOC_USAGE_SW_READ_OFTEN, 0, 0, size.width.as_int(), size.height.as_int(), _))
        .WillOnce(DoAll(SetArgPointee<7>(mapped_pixels.get()), Return(0)));
    EXPECT_CALL(gralloc, unlock_interface(_,_));

    mga::Buffer buffer(&gralloc, mock_native_buffer, extensions);
    buffer.read([](unsigned char const* pixels)
    {
        EXPECT_THAT(pixels, Ne(nullptr));
    });
    Mock::VerifyAndClearExpectations(&gralloc);
}
