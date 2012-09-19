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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl=mir::client;

struct MockAndroidRegistrar : public mcl::AndroidRegistrar
{
    MOCK_METHOD1(register_buffer, void(std::shared_ptr<mcl::MirBufferPackage>));
    MOCK_METHOD1(unregister_buffer, void(std::shared_ptr<mcl::MirBufferPackage>));
    MOCK_METHOD1(secure_for_cpu, char*(std::shared_ptr<mcl::MirBufferPackage>));
};

class ClientAndroidBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        package = std::make_shared<mcl::MirBufferPackage>();
        mock_android_registrar = std::make_shared<MockAndroidRegistrar>();
    }
    std::shared_ptr<mcl::MirBufferPackage> package;
    std::shared_ptr<mcl::AndroidClientBuffer> buffer;
    std::shared_ptr<MockAndroidRegistrar> mock_android_registrar;
};

TEST_F(ClientAndroidBufferTest, client_init)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, package);
    EXPECT_NE((int) buffer.get(), NULL);
}

TEST_F(ClientAndroidBufferTest, client_registers_right_handle)
{
    EXPECT_CALL(*mock_android_registrar, register_buffer(package))
        .Times(1);
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, package);
}

TEST_F(ClientAndroidBufferTest, client_registers_right_handle_resource_cleanup)
{
    EXPECT_CALL(*mock_android_registrar, register_buffer(package))
        .Times(1);
    EXPECT_CALL(*mock_android_registrar, unregister_buffer(package))
        .Times(1);
 
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, package);
}

TEST_F(ClientAndroidBufferTest, buffer_uses_registrar_for_secure)
{
    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, package);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(package))
        .Times(1);

    buffer->secure_for_cpu_write();
}

TEST_F(ClientAndroidBufferTest, buffer_acquires_vaddr_for_write)
{
    using namespace testing;

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, package);
    char * vaddr = (char*) 0x8534;

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(package))
        .Times(1)
        .WillOnce(Return(vaddr));

    auto region = buffer->secure_for_cpu_write();
    EXPECT_EQ(vaddr, region.vaddr);
}

TEST_F(ClientAndroidBufferTest, region_is_released)
{
    using namespace testing;

    buffer = std::make_shared<mcl::AndroidClientBuffer>(mock_android_registrar, package);

    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(package))
        .Times(1);

    auto region = buffer->secure_for_cpu_write();
    EXPECT_NE((int) region.get(), NULL);

}
