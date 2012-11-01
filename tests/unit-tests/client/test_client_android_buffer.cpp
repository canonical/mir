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

#include "mir_test/mock_android_registrar.h"
#include "mir_client/android/android_client_buffer.h"
#include "mir_client/mir_client_library.h"

#include <memory>
#include <algorithm>
#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt=mir::test;
namespace mcl=mir::client;
namespace geom=mir::geometry;

class ClientAndroidBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        
        height = geom::Height(124);
        height_copy = geom::Height(height);

        width = geom::Width(248);
        width_copy = geom::Width(width);

        size = geom::Size{width, height};

        stride = geom::Stride{66};

        pf = geom::PixelFormat::rgba_8888;
        pf_copy = geom::PixelFormat(pf);

        package = std::make_shared<MirBufferPackage>();
        package->stride = stride.as_uint32_t();

        mock_android_registrar = std::make_shared<mt::MockAndroidRegistrar>();
        package_copy = std::make_shared<MirBufferPackage>(*package.get());

        EXPECT_CALL(*mock_android_registrar, register_buffer(_))
            .Times(AtLeast(0));
        EXPECT_CALL(*mock_android_registrar, unregister_buffer(_))
            .Times(AtLeast(0));
    }

    std::shared_ptr<MirBufferPackage> package;
    std::shared_ptr<MirBufferPackage> package_copy;
    geom::Size size;
    geom::Height height, height_copy;
    geom::Width width, width_copy;
    geom::Stride stride;
    geom::PixelFormat pf, pf_copy;
    std::shared_ptr<mcl::AndroidClientBuffer> buffer;
    std::shared_ptr<mt::MockAndroidRegistrar> mock_android_registrar;
};

TEST_F(ClientAndroidBufferTest, client_buffer_assumes_ownership)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ((int) package.get(), NULL);
}

TEST_F(ClientAndroidBufferTest, client_buffer_converts_package_fd_correctly)
{
    using namespace testing;
    const native_handle_t *handle;

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&handle));
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    ASSERT_NE((int)handle, NULL);
    ASSERT_EQ(handle->numFds, (int) package_copy->fd_items);
    for(auto i = 0; i < package_copy->fd_items; i++)
        EXPECT_EQ(package_copy->fd[i], handle->data[i]); 
}

TEST_F(ClientAndroidBufferTest, client_buffer_converts_package_data_correctly)
{
    using namespace testing;
    const native_handle_t *handle;

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&handle));
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    ASSERT_NE((int)handle, NULL);
    ASSERT_EQ(handle->numInts, (int) package_copy->data_items);
    for(auto i = 0; i < package_copy->data_items; i++)
        EXPECT_EQ(package_copy->data[i], handle->data[i + package_copy->fd_items]); 
}

TEST_F(ClientAndroidBufferTest, client_registers_right_handle_resource_cleanup)
{
    using namespace testing;

    const native_handle_t* buffer_handle;
    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

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
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    int total = 3 + buffer_handle->numFds + buffer_handle->numInts;
    EXPECT_EQ(buffer_handle->version, total);
}

TEST_F(ClientAndroidBufferTest, buffer_uses_registrar_for_secure)
{
    using namespace testing;
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

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
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&buffer_handle_sp),
            Return(empty_char)));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(buffer_handle_sp.get(), buffer_handle);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_width_without_modifying)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(buffer->size().width, width_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_height_without_modifying)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(buffer->size().height, height_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_pf_without_modifying)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(buffer->pixel_format(), pf_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_correct_stride)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);
    EXPECT_EQ(buffer->stride(), stride);
}

TEST_F(ClientAndroidBufferTest, check_bounds_on_secure)
{
    using namespace testing;
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    geom::Point point{ geom::X(0), geom::Y(0)};
    geom::Size size{width_copy, height_copy};
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

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(region->vaddr.get(), empty_char.get());
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_width)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(region->width, width_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_height)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(region->height, height_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_stride)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(region->stride, stride);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_pf)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(region->format, pf_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    const native_handle_t* buffer_handle;
    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        size, pf);

    auto native_handle = buffer->get_native_handle();

    ASSERT_NE(native_handle, (ANativeWindowBuffer*) NULL);
    EXPECT_EQ(native_handle->handle, buffer_handle); 
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_dimensions)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        size, pf);

    auto native_handle = buffer->get_native_handle();

    ASSERT_NE(native_handle, (ANativeWindowBuffer*) NULL);
    EXPECT_EQ(native_handle->width,  (int) width_copy.as_uint32_t());
    EXPECT_EQ(native_handle->height, (int) height_copy.as_uint32_t());
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_format)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    auto native_handle = buffer->get_native_handle();
    int correct_usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    ASSERT_NE(native_handle, (ANativeWindowBuffer*) NULL);
    EXPECT_EQ(native_handle->usage,  correct_usage);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_refcounters_set)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    auto native_handle = buffer->get_native_handle();

    ASSERT_NE(native_handle, (ANativeWindowBuffer*) NULL);
    ASSERT_NE((int) native_handle->common.incRef,  NULL);
    ASSERT_NE((int) native_handle->common.decRef,  NULL);

    native_handle->common.incRef(NULL);
    native_handle->common.decRef(NULL);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_creation_package)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package), size, pf);

    auto package_return = buffer->get_buffer_package();

    EXPECT_EQ(package_return->data_items, package_copy->data_items);
    EXPECT_EQ(package_return->fd_items, package_copy->fd_items);
    EXPECT_EQ(package_return->stride, package_copy->stride);
    for(auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->data[i], package_copy->data[i]);
    for(auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->fd[i], package_copy->fd[i]);
}

