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

#include "mir_test_doubles/mock_android_registrar.h"
#include "src/client/android/android_client_buffer.h"
#include "mir_toolkit/mir_client_library.h"

#include <memory>
#include <algorithm>
#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mcl = mir::client;
namespace mcla = mir::client::android;
namespace geom = mir::geometry;

class ClientAndroidBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        height = geom::Height(124);
        width = geom::Width(248);
        size = geom::Size{width, height};
        stride = geom::Stride{66};
        pf = geom::PixelFormat::abgr_8888;

        mock_android_registrar = std::make_shared<NiceMock<mtd::MockAndroidRegistrar>>();

        package = std::make_shared<MirBufferPackage>();
        package->stride = stride.as_uint32_t();
    }

    std::shared_ptr<MirBufferPackage> package;
    geom::Size size;
    geom::Height height;
    geom::Width width;
    geom::Stride stride;
    geom::PixelFormat pf;
    std::shared_ptr<mcla::AndroidClientBuffer> buffer;
    std::shared_ptr<mtd::MockAndroidRegistrar> mock_android_registrar;
};

TEST_F(ClientAndroidBufferTest, client_buffer_converts_package_fd_correctly)
{
    using namespace testing;
    const native_handle_t *handle;

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&handle));

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    ASSERT_NE(nullptr, handle);
    ASSERT_EQ(package->fd_items, handle->numFds);
    for(auto i = 0; i < package->fd_items; i++)
        EXPECT_EQ(package->fd[i], handle->data[i]);
}

TEST_F(ClientAndroidBufferTest, client_buffer_converts_package_data_correctly)
{
    using namespace testing;
    const native_handle_t *handle;

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&handle));

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    ASSERT_NE(nullptr, handle);
    ASSERT_EQ(package->data_items, handle->numInts);
    for(auto i = 0; i < package->data_items; i++)
        EXPECT_EQ(package->data[i], handle->data[i + package->fd_items]);
}

TEST_F(ClientAndroidBufferTest, client_registers_right_handle_resource_cleanup)
{
    using namespace testing;

    const native_handle_t* buffer_handle;
    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, unregister_buffer(buffer_handle))
        .Times(1);
}

TEST_F(ClientAndroidBufferTest, client_sets_correct_version)
{
    using namespace testing;

    const native_handle_t* buffer_handle;
    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_EQ(buffer_handle->version, static_cast<int>(sizeof(native_handle_t)));
}

TEST_F(ClientAndroidBufferTest, buffer_uses_registrar_for_secure)
{
    using namespace testing;
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    std::shared_ptr<char> empty_char = std::make_shared<char>();
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));

    buffer->secure_for_cpu_write();
}

TEST_F(ClientAndroidBufferTest, buffer_uses_right_handle_to_secure)
{
    using namespace testing;
    const native_handle_t* buffer_handle;
    std::shared_ptr<const native_handle_t> buffer_handle_sp;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&buffer_handle_sp),
            Return(empty_char)));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(buffer_handle, buffer_handle_sp.get());
}

TEST_F(ClientAndroidBufferTest, buffer_returns_width_without_modifying)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(width, buffer->size().width);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_height_without_modifying)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(height, buffer->size().height);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_pf_without_modifying)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(pf, buffer->pixel_format());
}

TEST_F(ClientAndroidBufferTest, buffer_returns_correct_stride)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(stride, buffer->stride());
}

TEST_F(ClientAndroidBufferTest, check_bounds_on_secure)
{
    using namespace testing;
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    geom::Point point{geom::X(0), geom::Y(0)};
    geom::Size size{width, height};
    geom::Rectangle rect{point, size};

    std::shared_ptr<char> empty_char = std::make_shared<char>();
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,rect))
        .Times(1)
        .WillOnce(Return(empty_char));

    buffer->secure_for_cpu_write();
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_vaddr)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(empty_char, region->vaddr);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_width)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(width, region->width);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_height)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(height, region->height);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_stride)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(stride, region->stride);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_pf)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(pf, region->format);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer)
{
    using namespace testing;

    const native_handle_t* buffer_handle;
    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));

    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        size, pf);

    auto native_handle = buffer->get_native_handle();

    ASSERT_NE(nullptr, native_handle);
    EXPECT_EQ(buffer_handle, native_handle->handle);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_dimensions)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        size, pf);

    auto native_handle = buffer->get_native_handle();

    ASSERT_NE(nullptr, native_handle);
    EXPECT_EQ(width.as_uint32_t(), static_cast<uint32_t>(native_handle->width));
    EXPECT_EQ(height.as_uint32_t(), static_cast<uint32_t>(native_handle->height));
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_format)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    auto native_handle = buffer->get_native_handle();
    int correct_usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    ASSERT_NE(nullptr, native_handle);
    EXPECT_EQ(correct_usage, native_handle->usage);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_stride)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    auto native_handle = buffer->get_native_handle();
    ASSERT_NE(nullptr, native_handle);
    EXPECT_EQ(static_cast<int32_t>(stride.as_uint32_t()), native_handle->stride);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_refcounters_set)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    auto native_handle = buffer->get_native_handle();

    ASSERT_NE(nullptr, native_handle);
    ASSERT_NE(nullptr, native_handle->common.incRef);
    ASSERT_NE(nullptr, native_handle->common.decRef);

    native_handle->common.incRef(NULL);
    native_handle->common.decRef(NULL);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_creation_package)
{
    buffer = std::make_shared<mcla::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    auto package_return = buffer->get_buffer_package();

    EXPECT_EQ(package->data_items, package_return->data_items);
    EXPECT_EQ(package->fd_items, package_return->fd_items);
    EXPECT_EQ(package->stride, package_return->stride);
    for(auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package->data[i], package_return->data[i]);
    for(auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package->fd[i], package_return->fd[i]);
}

