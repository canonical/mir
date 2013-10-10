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
#include "src/server/graphics/android/buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/native_buffer.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_fence.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_native_buffer.h"

#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

class AndroidGraphicBufferBasic : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        mock_buffer_handle = mtd::create_stub_buffer();
        mock_buffer_handle->width = 44;
        mock_buffer_handle->height = 45;
        mock_buffer_handle->stride = 46;
        mock_buffer_handle->format = HAL_PIXEL_FORMAT_RGBA_8888;

        default_use = mga::BufferUsage::use_hardware;
        pf = geom::PixelFormat::abgr_8888;
        size = geom::Size{300, 200};
        extensions = std::make_shared<mg::EGLExtensions>();
        mock_sync_fence = std::make_shared<mtd::MockFence>();
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockFence> mock_sync_fence;
    std::shared_ptr<mga::NativeBuffer> mock_buffer_handle;
    geom::PixelFormat pf;
    geom::Size size;
    mga::BufferUsage default_use;
    std::shared_ptr<mg::EGLExtensions> extensions;
};

TEST_F(AndroidGraphicBufferBasic, size_query_test)
{
    using namespace testing;

    mga::Buffer buffer(mock_buffer_handle, mock_sync_fence, extensions);

    geom::Size expected_size{mock_buffer_handle->width, mock_buffer_handle->height};
    EXPECT_EQ(expected_size, buffer.size());
}

TEST_F(AndroidGraphicBufferBasic, format_query_test)
{
    using namespace testing;

    mga::Buffer buffer(mock_buffer_handle, mock_sync_fence, extensions);
    EXPECT_EQ(geom::PixelFormat::abgr_8888, buffer.pixel_format());
}

TEST_F(AndroidGraphicBufferBasic, returns_native_buffer_with_fence)
{
    using namespace testing;

    EXPECT_CALL(*mock_sync_fence, wait())
        .Times(1)

    mga::Buffer buffer(mock_buffer_handle, mock_sync_fence, extensions);
    auto native_resource = buffer.native_buffer_handle();

    EXPECT_EQ(mock_buffer_handle, native_resource);
    EXPECT_EQ(mock_sync_fence, native_resource->fence);
}

TEST_F(AndroidGraphicBufferBasic, returns_native_buffer_times_two)
{
    using namespace testing;
    int acquire_fake_fence_fd1 = 948;
    int acquire_fake_fence_fd2 = 954;

    EXPECT_CALL(*mock_sync_fence, wait())
        .Times(1)

    EXPECT_CALL(*mock_sync_fence, merge_with(acquire_fake_fence_fd1))
        .Times(1);
    EXPECT_CALL(*mock_sync_fence, merge_with(acquire_fake_fence_fd2))
        .Times(1);

    mga::Buffer buffer(mock_buffer_handle, mock_sync_fence, extensions);
    {
        auto native_resource = buffer.native_buffer_handle();
        EXPECT_EQ(mock_buffer_handle, native_resource);
        auto fence = native_resource->fence;
        fence->merge_with(acquire_fake_fence_fd1);
    }
    {
        auto native_resource = buffer.native_buffer_handle();
        EXPECT_EQ(mock_buffer_handle, native_resource);
        auto fence = native_resource->fence;
        fence->merge_with(acquire_fake_fence_fd2);
    }
}

TEST_F(AndroidGraphicBufferBasic, queries_native_window_for_stride)
{
    using namespace testing;

    geom::Stride expected_stride{mock_buffer_handle->stride *
                                 geom::bytes_per_pixel(pf)};
    mga::Buffer buffer(mock_buffer_handle, mock_sync_fence, extensions);
    EXPECT_EQ(expected_stride, buffer.stride());
}
