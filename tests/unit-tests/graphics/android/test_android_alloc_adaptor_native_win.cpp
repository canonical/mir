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

#include "src/server/graphics/android/android_buffer_handle_default.h"
#include "mir_test_doubles/mock_alloc_adaptor.h"
#include "mir_test_doubles/mock_android_alloc_device.h"

#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

class AdaptorNativeWinProduction : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_alloc_device = std::make_shared<mtd::MockAllocDevice>();

        /* set up common defaults */
        pf = geom::PixelFormat::abgr_8888;
        size = geom::Size{geom::Width{110}, geom::Height{230}};
        usage = mga::BufferUsage::use_hardware;
        stride = geom::Stride(300*4);

        anwb = std::make_shared<ANativeWindowBuffer>();
        anwb->height = (int) size.height.as_uint32_t();
        anwb->width = (int) size.width.as_uint32_t();
        anwb->stride = (int) stride.as_uint32_t();
        anwb->handle  = mock_alloc_device->buffer_handle;

    }

    std::shared_ptr<ANativeWindowBuffer> anwb;

    std::shared_ptr<mga::AndroidBufferHandleDefault> buffer_handle;
    std::shared_ptr<mtd::MockAllocDevice> mock_alloc_device;

    geom::Size size;
    geom::Stride stride;
    mga::BufferUsage usage;
    geom::PixelFormat pf;
};

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_size)
{
    buffer_handle =  std::make_shared<mga::AndroidBufferHandleDefault>(anwb, pf, usage );
    EXPECT_EQ(size, buffer_handle->size());
}

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_stride)
{
    buffer_handle =  std::make_shared<mga::AndroidBufferHandleDefault>(anwb, pf, usage );

    EXPECT_EQ(stride, buffer_handle->stride());
}

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_format)
{
    buffer_handle =  std::make_shared<mga::AndroidBufferHandleDefault>(anwb, pf, usage );
    EXPECT_EQ(pf, buffer_handle->format());
}

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_usage)
{
    buffer_handle =  std::make_shared<mga::AndroidBufferHandleDefault>(anwb, pf, usage );
    EXPECT_EQ(usage, buffer_handle->usage());
}

TEST_F(AdaptorNativeWinProduction, returns_handle_non_null)
{
    buffer_handle = std::make_shared<mga::AndroidBufferHandleDefault>(anwb, pf, usage );
    auto handle = buffer_handle->native_buffer_handle();
    ASSERT_NE(nullptr, handle);
}

TEST_F(AdaptorNativeWinProduction, returns_handle_with_no_modifications)
{
    anwb->height = 11;
    anwb->width =  22;
    anwb->stride = 33;
    anwb->format = 44;
    anwb->usage =  55;

    buffer_handle = std::make_shared<mga::AndroidBufferHandleDefault>(anwb, pf, usage );
    auto handle = buffer_handle->native_buffer_handle();

    EXPECT_EQ(anwb->width, handle->width);
    EXPECT_EQ(anwb->height, handle->height);
    EXPECT_EQ(anwb->stride, handle->stride);
    EXPECT_EQ(anwb->usage, handle->usage);
    EXPECT_EQ(anwb->format, handle->format);
    EXPECT_EQ(anwb->handle, handle->handle);

}
