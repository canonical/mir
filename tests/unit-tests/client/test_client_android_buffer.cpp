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

#include "mir_client/android_client_buffer.h"
#include "mir_client/mir_buffer_package.h"

#include <memory>
#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl=mir::client;
namespace geom=mir::geometry;

struct MockAndroidRegistrar : public mcl::AndroidRegistrar
{
    MOCK_METHOD1(register_buffer,   void(const native_handle_t*));
    MOCK_METHOD1(unregister_buffer, void(const native_handle_t*));
    MOCK_METHOD2(secure_for_cpu, std::shared_ptr<char>(std::shared_ptr<const native_handle_t>, geom::Rectangle));
};

class ClientAndroidBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        height = geom::Height(124);
        height_copy = geom::Height(height);

        width = geom::Width(248);
        width_copy = geom::Width(width);

        pf = geom::PixelFormat::rgba_8888;
        pf_copy = geom::PixelFormat(pf);

        package = std::make_shared<mcl::MirBufferPackage>();
        mock_android_registrar = std::make_shared<MockAndroidRegistrar>();
        package_copy = std::make_shared<mcl::MirBufferPackage>(*package.get());
    }
    std::shared_ptr<mcl::MirBufferPackage> package;
    std::shared_ptr<mcl::MirBufferPackage> package_copy;
    geom::Height height, height_copy;
    geom::Width width, width_copy;
    geom::PixelFormat pf, pf_copy;
    std::shared_ptr<mcl::AndroidClientBuffer> buffer;
    std::shared_ptr<MockAndroidRegistrar> mock_android_registrar;
};

TEST_F(ClientAndroidBufferTest, client_init)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));
    EXPECT_NE((int) buffer.get(), NULL);
}

TEST_F(ClientAndroidBufferTest, client_buffer_assumes_ownership)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));
    EXPECT_EQ((int) package.get(), NULL);
}

TEST_F(ClientAndroidBufferTest, client_buffer_converts_package_fd_correctly)
{
    using namespace testing;
    const native_handle_t *handle;
    int i=0;

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&handle));
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

    ASSERT_NE((int)handle, NULL);
    ASSERT_EQ(handle->numFds, (int) package_copy->fd.size());
    for(auto it = package_copy->fd.begin(); it != package_copy->fd.end(); it++)
        EXPECT_EQ(*it, handle->data[i++]); 
}

TEST_F(ClientAndroidBufferTest, client_buffer_converts_package_data_correctly)
{
    using namespace testing;
    const native_handle_t *handle;
    int i=0;

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&handle));
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

    ASSERT_NE((int)handle, NULL);
    ASSERT_EQ(handle->numInts, (int) package_copy->data.size());
    for(auto it = package_copy->fd.begin(); it != package_copy->fd.end(); it++)
        EXPECT_EQ(*it, handle->data[i++]); 
}

TEST_F(ClientAndroidBufferTest, client_registers_right_handle_resource_cleanup)
{
    using namespace testing;

    const native_handle_t* buffer_handle;
    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

    EXPECT_CALL(*mock_android_registrar, unregister_buffer(buffer_handle))
        .Times(1);
}

TEST_F(ClientAndroidBufferTest, buffer_uses_registrar_for_secure)
{
    using namespace testing;
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

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
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&buffer_handle_sp),
            Return(empty_char)));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(buffer_handle_sp.get(), buffer_handle);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_width_without_modifying)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));
    EXPECT_EQ(buffer->width(), width_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_height_without_modifying)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));
    EXPECT_EQ(buffer->height(), height_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_returns_pf_without_modifying)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));
    EXPECT_EQ(buffer->pixel_format(), pf_copy);
}

TEST_F(ClientAndroidBufferTest, check_bounds_on_secure)
{
    using namespace testing;
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

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

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

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

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

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

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(region->height, height_copy);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_pf)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package),
                                                        std::move(width), std::move(height), std::move(pf));

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));
    auto region = buffer->secure_for_cpu_write();

    EXPECT_EQ(region->format, pf_copy);
}
