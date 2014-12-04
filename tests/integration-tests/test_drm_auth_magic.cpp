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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/buffer_basic.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_drm.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace
{

char const* const mir_test_socket = mtf::test_socket_file().c_str();

struct MockAuthenticatingIpcOps : public mg::PlatformIpcOperations
{
    MOCK_CONST_METHOD3(pack_buffer, void(mg::BufferIpcMessage&, mg::Buffer const&, mg::BufferIpcMsgType));
    MOCK_CONST_METHOD2(unpack_buffer, void(mg::BufferIpcMessage&, mg::Buffer const&));
    MOCK_METHOD0(connection_ipc_package, std::shared_ptr<mg::PlatformIPCPackage>());
    MOCK_METHOD2(platform_operation, mg::PlatformOperationMessage(
        unsigned int const, mg::PlatformOperationMessage const&));
};

class StubAuthenticatingPlatform : public mtd::NullPlatform
{
public:
    StubAuthenticatingPlatform(std::shared_ptr<mg::PlatformIpcOperations> const& ops) :
        ops{ops}
    {
    }

    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator() override
    {
        return std::make_shared<mtd::StubBufferAllocator>();
    }
    std::shared_ptr<mg::PlatformIpcOperations> make_ipc_operations() const override
    {
        return ops;
    }

private:
    std::shared_ptr<mg::PlatformIpcOperations> const ops;
};

void connection_callback(MirConnection* connection, void* context)
{
    auto connection_ptr = static_cast<MirConnection**>(context);
    *connection_ptr = connection;
}

void drm_auth_magic_callback(int status, void* client_context)
{
    auto status_ptr = static_cast<int*>(client_context);
    *status_ptr = status;
}

}

TEST_F(BespokeDisplayServerTestFixture, client_drm_auth_magic_calls_platform)
{
    unsigned int const magic{0x10111213};

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform()
        {
            using namespace testing;
            if (!platform)
            {
                mg::PlatformOperationMessage pkg{{0},{}};
                auto ipc_ops = std::make_shared<NiceMock<MockAuthenticatingIpcOps>>();
                EXPECT_CALL(*ipc_ops, platform_operation(_,_))
                    .Times(1)
                    .WillRepeatedly(Return(pkg));
                ON_CALL(*ipc_ops, connection_ipc_package())
                    .WillByDefault(Return(std::make_shared<mg::PlatformIPCPackage>()));
                platform = std::make_shared<StubAuthenticatingPlatform>(ipc_ops);
            }

            return platform;
        }

        std::shared_ptr<StubAuthenticatingPlatform> platform;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            MirConnection* connection{nullptr};
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__,
                                     connection_callback, &connection));

            int const no_error{0};
            int status{67};

            ASSERT_TRUE(mir_connection_is_valid(connection));

            mir_wait_for(mir_connection_drm_auth_magic(connection, magic,
                                                       drm_auth_magic_callback,
                                                       &status));
            EXPECT_EQ(no_error, status);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(BespokeDisplayServerTestFixture, drm_auth_magic_platform_error_reaches_client)
{
    unsigned int const magic{0x10111213};
    static int const auth_magic_error{667};

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform()
        {
            using namespace testing;
            if (!platform)
            {
                auto ipc_ops = std::make_shared<NiceMock<MockAuthenticatingIpcOps>>();
                EXPECT_CALL(*ipc_ops, platform_operation(_,_))
                    .WillOnce(Throw(::boost::enable_error_info(std::exception())
                        << boost::errinfo_errno(auth_magic_error)));
                platform = std::make_shared<StubAuthenticatingPlatform>(ipc_ops);
            }

            return platform;
        }

        std::shared_ptr<StubAuthenticatingPlatform> platform;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            MirConnection* connection{nullptr};
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__,
                                     connection_callback, &connection));

            int status{67};

            mir_wait_for(mir_connection_drm_auth_magic(connection, magic,
                                                       drm_auth_magic_callback,
                                                       &status));
            EXPECT_EQ(auth_magic_error, status);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
