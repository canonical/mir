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

struct MockAndroidRegistrar : public mcl::AndroidRegistrar
{
    MOCK_METHOD1(register_buffer,   void(const native_handle_t*));
    MOCK_METHOD1(unregister_buffer, void(const native_handle_t*));
    MOCK_METHOD1(secure_for_cpu, std::shared_ptr<mcl::MemoryRegion>(std::shared_ptr<const native_handle_t>));
};

class ClientAndroidBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        package = std::make_shared<mcl::MirBufferPackage>();
        mock_android_registrar = std::make_shared<MockAndroidRegistrar>();
        package_copy = std::make_shared<mcl::MirBufferPackage>(*package.get());
    }
    std::shared_ptr<mcl::MirBufferPackage> package;
    std::shared_ptr<mcl::MirBufferPackage> package_copy;
    std::shared_ptr<mcl::AndroidClientBuffer> buffer;
    std::shared_ptr<MockAndroidRegistrar> mock_android_registrar;
};

TEST_F(ClientAndroidBufferTest, client_init)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package));
    EXPECT_NE((int) buffer.get(), NULL);
}

TEST_F(ClientAndroidBufferTest, client_buffer_assumes_ownership)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package));
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
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package));

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
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package));

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
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package));

    EXPECT_CALL(*mock_android_registrar, unregister_buffer(buffer_handle))
        .Times(1);
}

TEST_F(ClientAndroidBufferTest, buffer_uses_registrar_for_secure)
{
    using namespace testing;
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package));

    std::shared_ptr<mcl::MemoryRegion> empty_region;
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_))
        .Times(1)
        .WillOnce(Return(empty_region));

    buffer->secure_for_cpu_write();
}

TEST_F(ClientAndroidBufferTest, DISABLED_buffer_uses_right_handle_to_secure)
{
    using namespace testing;
    const native_handle_t* buffer_handle;
    std::shared_ptr<const native_handle_t> buffer_handle_sp;

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, std::move(package));

    EXPECT_CALL(*mock_android_registrar, register_buffer(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&buffer_handle));

    auto region = buffer->secure_for_cpu_write();

    std::shared_ptr<mcl::MemoryRegion> empty_region;
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&buffer_handle_sp),
            Return(empty_region)));

    EXPECT_EQ(buffer_handle_sp.get(), buffer_handle);
}

