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

#include "mir_protobuf.pb.h"
#include "mir_client/mir_client_library.h"
#include "mir_client/mir_logger.h"
#include "client_buffer.h"
#include "client_buffer_factory.h"
#include "mir_rpc_channel.h"
#include "mir_buffer_package.h"
#include "mir_surface.h"
#include "mir/frontend/resource_cache.h"

#include "mir_test/test_server.h"
#include "mir_test/mock_server_tool.h"
#include "mir_test/test_client.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;

namespace mir
{
namespace test
{ 

struct MockServerPackageGenerator : public MockServerTool
{
    MockServerPackageGenerator()
    {
        generate_unique_buffer();
    }

    void create_surface(google::protobuf::RpcController* ,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        create_surface_response( response );
        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        done->Run();
    }

    void next_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::SurfaceId const* /*request*/,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done)
    {
        create_buffer_response( response );
        done->Run();
    }

/* helpers */
    void generate_unique_buffer()
    {
        int num_fd = 2, num_data = 8; 
        server_package.fd.clear();
        server_package.data.clear();
        server_package.fd.resize(num_fd);
        server_package.data.resize(num_data);
        for (auto i=0; i<num_fd; i++)
        {
            server_package.fd[i] = i*3;
        }
        for (auto i=0; i<num_data; i++)
        {
            server_package.data[i] = i*2;
        }
    }
 
    mcl::MirBufferPackage server_package;

    private:
    void create_buffer_response(mir::protobuf::Buffer* response)
    {
        response->set_buffer_id(0);

        /* assemble buffers */
        response->set_fds_on_side_channel(1);
        for (unsigned int i=0; i< server_package.data.size(); i++)
        {
            response->add_data(server_package.data[i]);
        }
        for (unsigned int i=0; i< server_package.fd.size(); i++)
        {
            response->add_fd(server_package.fd[i]);
        }
    }

    void create_surface_response(mir::protobuf::Surface* response)
    {
        response->mutable_id()->set_value(2);
        response->set_width(3);
        response->set_height(5);
        response->set_pixel_format(mir_pixel_format_rgba_8888);
        create_buffer_response(response->mutable_buffer());
    }
};

struct MockBuffer : public mcl::ClientBuffer
{
    MockBuffer()
    {

    }

    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_METHOD0(width, geom::Width());
    MOCK_METHOD0(height, geom::Height());
    MOCK_METHOD0(pixel_format, geom::PixelFormat());
};

struct MockClientFactory : public mcl::ClientBufferFactory
{
    MockClientFactory()
    {
        using namespace testing;
        ON_CALL(*this, create_buffer_from_ipc_message(_))
            .WillByDefault(Return(emptybuffer));
    }
    MOCK_METHOD1(create_buffer_from_ipc_message, std::shared_ptr<mcl::ClientBuffer>(const mcl::MirBufferPackage&));

    std::shared_ptr<mcl::ClientBuffer> emptybuffer;
};

}
}

namespace mt = mir::test;



struct CallBack
{
    void msg()
    {
        printf("SERVER CONNECT\n");
    }
};

struct MirClientSurfaceTest : public testing::Test
{
    void SetUp()
    {

        mock_server_tool = std::make_shared<mt::MockServerPackageGenerator>();
        test_server = std::make_shared<mt::TestServer>("./test_socket_surface", mock_server_tool);
        test_server->comm.start();

        mock_factory = std::make_shared<mt::MockClientFactory>();

        params = MirSurfaceParameters{"test", 33, 45, mir_pixel_format_rgba_8888};
  
 
        /* connect dummy server */
        connect_parameters.set_application_name("test");
        mock_server_tool->connect(0,
                        &connect_parameters,
                        &response,
                        google::protobuf::NewCallback(&callback, &CallBack::msg));

        /* connect client */
        logger = std::make_shared<mcl::ConsoleLogger>();
        channel = std::make_shared<mcl::MirRpcChannel>(std::string("./test_socket_surface"), logger); 
        client_comm_channel = std::make_shared<mir::protobuf::DisplayServer::Stub>(channel.get());
        client_comm_channel->connect(
            0,
            &connect_parameters,
            &response,
            google::protobuf::NewCallback(&callback, &CallBack::msg));
    }

    void TearDown()
    {
        test_server->comm.stop();
    }

    std::shared_ptr<mcl::MirRpcChannel> channel;
    std::shared_ptr<mcl::ConsoleLogger> logger;

    MirSurfaceParameters params;
    std::shared_ptr<mt::MockClientFactory> mock_factory;

    mir::protobuf::Connection response;
    mir::protobuf::ConnectParameters connect_parameters;

    std::shared_ptr<mt::TestServer> test_server;
    std::shared_ptr<mt::TestClient> client_tools;
    std::shared_ptr<mt::MockServerPackageGenerator> mock_server_tool;

    CallBack callback;

    std::shared_ptr<mir::protobuf::DisplayServer::Stub> client_comm_channel;
};

void empty_callback(MirSurface*, void*) { }
TEST_F(MirClientSurfaceTest, client_buffer_created_on_surface_creation )
{
    using namespace testing;

    EXPECT_CALL(*mock_factory, create_buffer_from_ipc_message(_))
        .Times(1);
 
    auto surface = std::make_shared<MirSurface> ( *client_comm_channel, mock_factory, params, &empty_callback, (void*) NULL);
    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_result();
}

TEST_F(MirClientSurfaceTest, client_buffer_uses_ipc_message_from_server_on_create )
{
    using namespace testing;

    mcl::MirBufferPackage submitted_package;
    EXPECT_CALL(*mock_factory, create_buffer_from_ipc_message(_))
        .Times(1)
        .WillOnce(DoAll(
            SaveArg<0>(&submitted_package),
            Return(mock_factory->emptybuffer)));
 
    auto surface = std::make_shared<MirSurface> (
                         *client_comm_channel, mock_factory, params, &empty_callback, (void*) NULL);
    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_result();

    /* check for same contents */
    ASSERT_EQ(submitted_package.data.size(), mock_server_tool->server_package.data.size());
    ASSERT_EQ(submitted_package.fd.size(),   mock_server_tool->server_package.fd.size());
    for(unsigned int i=0; i< submitted_package.data.size(); i++)
        EXPECT_EQ(submitted_package.data[i], mock_server_tool->server_package.data[i]);
}

void empty_surface_callback(MirSurface*, void*) {}

TEST_F(MirClientSurfaceTest, client_does_not_create_a_buffer_its_seen_before )
{
    using namespace testing;

    /* setup */
    auto surface = std::make_shared<MirSurface> ( *client_comm_channel, mock_factory, params, &empty_callback, (void*) NULL);
    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_result();
    
    /* test */
    EXPECT_CALL(*mock_factory, create_buffer_from_ipc_message(_))
        .Times(0);
    auto buffer_wait_handle = surface->next_buffer(&empty_surface_callback, (void*) NULL);
    buffer_wait_handle->wait_for_result();
}

TEST_F(MirClientSurfaceTest, client_buffer_created_on_next_unique_buffer )
{
    using namespace testing;

    /* setup */
    auto surface = std::make_shared<MirSurface> ( *client_comm_channel, mock_factory, params, &empty_callback, (void*) NULL);
    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_result();

    /* test */
    EXPECT_CALL(*mock_factory, create_buffer_from_ipc_message(_))
        .Times(1);
    auto buffer_wait_handle = surface->next_buffer(&empty_surface_callback, (void*) NULL);
    buffer_wait_handle->wait_for_result();
}

TEST_F(MirClientSurfaceTest, client_buffer_uses_ipc_message_from_server_on_next_unique_buffer )
{
    using namespace testing;

    mcl::MirBufferPackage submitted_package;
 
    auto surface = std::make_shared<MirSurface> (
                         *client_comm_channel, mock_factory, params, &empty_callback, (void*) NULL);
    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_result();

    /* gen new buffer for next call*/
    mock_server_tool->generate_unique_buffer();
    EXPECT_CALL(*mock_factory, create_buffer_from_ipc_message(_))
        .Times(1)
        .WillOnce(DoAll(
            SaveArg<0>(&submitted_package),
            Return(mock_factory->emptybuffer)));

    /* request new */
    auto buffer_wait_handle = surface->next_buffer(&empty_surface_callback, (void*) NULL);
    buffer_wait_handle->wait_for_result();

    /* check for same contents */
    ASSERT_EQ(submitted_package.data.size(), mock_server_tool->server_package.data.size());
    ASSERT_EQ(submitted_package.fd.size(),   mock_server_tool->server_package.fd.size());
    for(unsigned int i=0; i< submitted_package.data.size(); i++)
        EXPECT_EQ(submitted_package.data[i], mock_server_tool->server_package.data[i]);
}
