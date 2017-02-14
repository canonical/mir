/*
 * Copyright © 2017 Canonical Ltd.
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

#include "src/server/graphics/nested/host_buffer.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/mir_connection.h"
#include "src/client/buffer_factory.h"
#include "src/client/buffer.h"
#include "mir/test/stub_server_tool.h"
#include "mir/test/test_protobuf_server.h"
#include "mir/test/doubles/stub_client_platform_factory.h"
#include "mir/test/doubles/mock_client_platform.h"
#include "mir/frontend/connector.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fcntl.h>

using namespace testing;
namespace mgn = mir::graphics::nested;
namespace mt = mir::test;
namespace mcl = mir::client;
namespace mtd = mir::test::doubles;

int b = 4;

std::unique_ptr<mir::Fd> fd;
int get_fence(MirBuffer*)
{
    if (fd)
        return *fd;
    return -1;
}
struct TestConnectionConfiguration : mir::client::DefaultConnectionConfiguration
{
public:

    TestConnectionConfiguration()
        : DefaultConnectionConfiguration("./test_socket_surface")
    {
    }

    std::shared_ptr<mcl::ClientPlatformFactory> the_client_platform_factory() override
    {
        printf("ZA\n");
        auto platform = std::make_shared<mtd::MockClientPlatform>();
        ON_CALL(*platform, request_interface(StrEq("mir_extension_fenced_buffers"), 1))
            .WillByDefault(Return(&fence_ext));
        return std::make_shared<mtd::StubClientPlatformFactory>(platform);
    }

    MirExtensionFencedBuffersV1 fence_ext{get_fence, nullptr, nullptr};

    std::shared_ptr<mir::client::AsyncBufferFactory> the_buffer_factory() override
    {
        struct StubBufferFactory : mcl::AsyncBufferFactory
        {
            std::shared_ptr<mcl::Buffer> buffer;
            std::unique_ptr<mcl::MirBuffer> generate_buffer(mir::protobuf::Buffer const&) override
            {
                return nullptr;
            }

            void expect_buffer(
                std::shared_ptr<mcl::ClientBufferFactory> const&,
                MirConnection* conn,
                mir::geometry::Size,
                MirPixelFormat,
                MirBufferUsage usage,
                MirBufferCallback cb,
                void* ctx) override
            {
                if (!buffer)
                    buffer = std::make_shared<mcl::Buffer>(cb, ctx, 1, nullptr, conn, usage);



                printf("FIRE CB\n");
                cb((MirBuffer*)buffer.get(), ctx);
                printf("GAA\n");
            }

            void expect_buffer(
                std::shared_ptr<mcl::ClientBufferFactory> const&,
                MirConnection*,
                mir::geometry::Size,
                uint32_t, uint32_t,
                MirBufferCallback, void*) override
            {
            }

            void cancel_requests_with_context(void*) {}
        };
        return std::make_shared<StubBufferFactory>(); 
    }
};

struct HostBuffer : Test
{
    HostBuffer()
    {
        //MirConnection is hard to stub
        std::remove("./test_socket_surface");
        test_server = std::make_shared<mt::TestProtobufServer>("./test_socket_surface", server_tool);
        test_server->comm->start();
        config = std::make_shared<TestConnectionConfiguration>();
    }

    std::shared_ptr<mt::StubServerTool> const server_tool = std::make_shared<mt::StubServerTool>();
    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<TestConnectionConfiguration> config;
};

//LP: #1664562
TEST_F(HostBuffer, does_not_own_fd_when_accessing_fence)
{
    fd = std::make_unique<mir::Fd>( fileno(tmpfile()) );

    MirConnection connection{*config};
    connection.connect("", [](auto*, auto*){printf("CONN\n");}, nullptr)->wait_for_all();
    mgn::HostBuffer buffer(&connection, mir::geometry::Size{1,1}, mir_pixel_format_abgr_8888);

    {
        auto fence = buffer.fence();
        EXPECT_THAT(fence, Eq(*fd));
    }

    EXPECT_THAT(fcntl(*fd, F_GETFD), Eq(0));

    fd.reset();
}
