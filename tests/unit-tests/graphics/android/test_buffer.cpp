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
#include "src/platform/graphics/android/buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_fence.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_android_native_buffer.h"

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
        mock_native_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();

        anwb = mock_native_buffer->anwb();
        anwb->width = 44;
        anwb->height = 45;
        anwb->stride = 46;
        anwb->format = HAL_PIXEL_FORMAT_RGBA_8888;

        default_use = mga::BufferUsage::use_hardware;
        pf = mir_pixel_format_abgr_8888;
        size = geom::Size{300, 200};
        extensions = std::make_shared<mg::EGLExtensions>();
    }

    ANativeWindowBuffer *anwb;
    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> mock_native_buffer;
    MirPixelFormat pf;
    geom::Size size;
    mga::BufferUsage default_use;
    std::shared_ptr<mg::EGLExtensions> extensions;
};

TEST_F(AndroidGraphicBufferBasic, size_query_test)
{
    using namespace testing;

    mga::Buffer buffer(mock_native_buffer, extensions);

    geom::Size expected_size{anwb->width, anwb->height};
    EXPECT_EQ(expected_size, buffer.size());
}

TEST_F(AndroidGraphicBufferBasic, format_query_test)
{
    using namespace testing;

    mga::Buffer buffer(mock_native_buffer, extensions);
    EXPECT_EQ(mir_pixel_format_abgr_8888, buffer.pixel_format());
}

TEST_F(AndroidGraphicBufferBasic, returns_native_buffer_times_two)
{
    using namespace testing;
    int acquire_fake_fence_fd1 = 948;
    int acquire_fake_fence_fd2 = 954;

    EXPECT_CALL(*mock_native_buffer, update_fence(acquire_fake_fence_fd1))
        .Times(1);
    EXPECT_CALL(*mock_native_buffer, update_fence(acquire_fake_fence_fd2))
        .Times(1);

    mga::Buffer buffer(mock_native_buffer, extensions);
    {
        auto native_resource = buffer.native_buffer_handle();
        EXPECT_EQ(mock_native_buffer, native_resource);
        native_resource->update_fence(acquire_fake_fence_fd1);
    }
    {
        auto native_resource = buffer.native_buffer_handle();
        EXPECT_EQ(mock_native_buffer, native_resource);
        native_resource->update_fence(acquire_fake_fence_fd2);
    }
}

TEST_F(AndroidGraphicBufferBasic, queries_native_window_for_stride)
{
    using namespace testing;

    geom::Stride expected_stride{anwb->stride *
                                 MIR_BYTES_PER_PIXEL(pf)};
    mga::Buffer buffer(mock_native_buffer, extensions);
    EXPECT_EQ(expected_stride, buffer.stride());
}
