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

#include "mir/test/doubles/mock_buffer_registrar.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "src/platforms/android/client/buffer.h"
#include "mir_toolkit/mir_client_library.h"

#include <memory>
#include <algorithm>
#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace mcla = mir::client::android;
namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

struct AndroidClientBuffer : public ::testing::Test
{
    AndroidClientBuffer() :
        mock_registrar{std::make_shared<testing::NiceMock<mtd::MockBufferRegistrar>>()},
        native_handle{std::make_shared<native_handle_t>()},
        mock_native_buffer{std::make_shared<mtd::MockAndroidNativeBuffer>(size)}
    {
        ON_CALL(*mock_registrar, register_buffer(testing::_, testing::_))
            .WillByDefault(testing::Return(mock_native_buffer));
        package.height = height.as_int();
        package.width = width.as_int();
        package.stride = stride.as_int();
    }

    geom::Height const height{124};
    geom::Width const width{248};
    geom::Size const size{width, height};
    geom::Stride const stride{66};
    MirPixelFormat const pf{mir_pixel_format_abgr_8888};
    std::shared_ptr<mtd::MockBufferRegistrar> const mock_registrar;
    std::shared_ptr<native_handle_t const> const native_handle;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> const mock_native_buffer;
    MirBufferPackage package;
};

TEST_F(AndroidClientBuffer, registers_native_handle_with_correct_size)
{
    EXPECT_CALL(*mock_registrar, register_buffer(testing::_, testing::_))
        .WillOnce(testing::Return(mock_native_buffer));

    mcla::Buffer buffer(mock_registrar, package, pf);
    EXPECT_EQ(size, buffer.size());
    EXPECT_EQ(pf, buffer.pixel_format());
    EXPECT_EQ(stride, buffer.stride());
}

TEST_F(AndroidClientBuffer, packs_memory_region_correctly)
{
    using namespace testing;
    geom::Rectangle rect{{0,0}, size};
    std::shared_ptr<char> empty_char = std::make_shared<char>();
    EXPECT_CALL(*mock_registrar, secure_for_cpu(Eq(mock_native_buffer),rect))
        .Times(1)
        .WillOnce(Return(empty_char));

    mcla::Buffer buffer(mock_registrar, package, pf);
    auto region = buffer.secure_for_cpu_write();
    EXPECT_EQ(empty_char, region->vaddr);
    EXPECT_EQ(width, region->width);
    EXPECT_EQ(height, region->height);
    EXPECT_EQ(stride, region->stride);
    EXPECT_EQ(pf, region->format);
}

TEST_F(AndroidClientBuffer, update_from_package_merges_fence_when_present)
{
    mga::NativeFence fake_fence{213};
    EXPECT_CALL(*mock_native_buffer, update_usage(fake_fence, mga::BufferAccess::read))
        .Times(1);
    mcla::Buffer buffer(mock_registrar, package, pf);

    package.data_items = 1;
    package.fd_items = 1;
    package.data[0] = static_cast<int>(mga::BufferFlag::fenced);
    package.fd[0] = fake_fence;
    buffer.update_from(package);
 
    package.data[0] = static_cast<int>(mga::BufferFlag::unfenced);
    buffer.update_from(package);
}

TEST_F(AndroidClientBuffer, fills_update_msg)
{
    using namespace testing;
    using mir::graphics::android::BufferFlag;
    int stub_fence{44};
    int invalid_fence{-1};

    EXPECT_CALL(*mock_native_buffer, copy_fence())
        .Times(2)
        .WillOnce(Return(stub_fence))
        .WillOnce(Return(invalid_fence));

    MirBufferPackage msg;
    mcla::Buffer buffer(mock_registrar, package, pf);

    buffer.fill_update_msg(msg);

    EXPECT_THAT(msg.data_items, Eq(1));
    EXPECT_THAT(msg.data[0], Eq(static_cast<int>(BufferFlag::fenced)));
    EXPECT_THAT(msg.fd_items, Eq(1));
    EXPECT_THAT(msg.fd[0], Eq(stub_fence));

    buffer.fill_update_msg(msg);

    EXPECT_THAT(msg.data_items, Eq(1));
    EXPECT_THAT(msg.data[0], Eq(static_cast<int>(BufferFlag::unfenced)));
    EXPECT_THAT(msg.fd_items, Eq(0));
}
