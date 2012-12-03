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

#include "mir_client/mir_logger.h"
#include "mir_client/client_platform.h"
#include "mir_client/client_platform_factory.h"
#include "mir_client/mir_connection.h"
#include "mir_client/make_rpc_channel.h"

#include "mir/frontend/resource_cache.h" /* needed by test_server.h */
#include "mir_test/test_protobuf_server.h"
#include "mir_test/stub_server_tool.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mt = mir::test;

namespace
{

struct MockClientPlatform : public mcl::ClientPlatform
{
    MOCK_METHOD0(create_platform_depository, std::shared_ptr<mcl::ClientBufferDepository>());
    MOCK_METHOD1(create_egl_native_window, std::shared_ptr<EGLNativeWindowType>(mcl::ClientSurface*));
    MOCK_METHOD0(create_egl_native_display, std::shared_ptr<EGLNativeDisplayType>());
};

struct MockClientPlatformFactory : public mcl::ClientPlatformFactory
{
    MOCK_METHOD1(create_client_platform, std::shared_ptr<mcl::ClientPlatform>(mcl::ClientContext*));
};

void connected_callback(MirConnection* /*connection*/, void * /*client_context*/)
{
}

}

struct MirConnectionTest : public testing::Test
{
    void SetUp()
    {
        using namespace testing;

        /* Set up test server */
        server_tool = std::make_shared<mt::StubServerTool>();
        test_server = std::make_shared<mt::TestProtobufServer>("./test_socket_surface", server_tool);

        EXPECT_CALL(*test_server->factory, make_ipc_server()).Times(testing::AtLeast(0));
        test_server->comm->start();

        /* Create MirConnection dependencies */
        logger = std::make_shared<mcl::ConsoleLogger>();
        platform = std::make_shared<NiceMock<MockClientPlatform>>();
        platform_factory = std::make_shared<NiceMock<MockClientPlatformFactory>>();

        /* Set up return values for mock objects methods */
        ON_CALL(*platform_factory, create_client_platform(_))
            .WillByDefault(Return(platform));

        connection = std::make_shared<MirConnection>(
            mcl::make_rpc_channel("./test_socket_surface", logger),
            logger,
            platform_factory);
    }

    void TearDown()
    {
        test_server.reset();
    }

    std::shared_ptr<mcl::Logger> logger;
    std::shared_ptr<testing::NiceMock<MockClientPlatform>> platform;
    std::shared_ptr<testing::NiceMock<MockClientPlatformFactory>> platform_factory;
    std::shared_ptr<MirConnection> connection;

    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<mt::StubServerTool> server_tool;
};

TEST_F(MirConnectionTest, returns_correct_egl_native_display)
{
    using namespace testing;

    EGLNativeDisplayType native_display_raw = reinterpret_cast<EGLNativeDisplayType>(0xabcdef);
    auto native_display = std::make_shared<EGLNativeDisplayType>();
    *native_display = native_display_raw;

    EXPECT_CALL(*platform, create_egl_native_display())
        .WillOnce(Return(native_display));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_result();

    EGLNativeDisplayType connection_native_display = connection->egl_native_display();

    ASSERT_EQ(native_display_raw, connection_native_display);
}
