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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"
#include "mir_rpc_channel.h"

#include "mir_test/mock_ipc_factory.h"
#include "mir_test/mock_logger.h"
#include "mir_test/mock_server_tool.h"
#include "mir_test/test_client.h"
#include "mir_test/test_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <memory>
#include <string>

namespace mf = mir::frontend;
namespace mt = mir::test;

namespace mir
{
namespace test
{

struct MockServerSurfaceCounter : public MockServerTool
{
    int surface_count;

    MockServerSurfaceCounter() : surface_count(0)
    {
    }

    void create_surface(google::protobuf::RpcController* controller,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    { 
        ++surface_count;
        MockServerTool::create_surface(controller, request, response, done);
    }
    
    void expect_surface_count(int expected_count)
    {
        std::unique_lock<std::mutex> ul(guard);
        while (surface_count != expected_count)
            wait_condition.wait(ul);

        EXPECT_EQ(expected_count, surface_count);
    }
};

}

struct ProtobufAsioCommunicatorCounter : public ::testing::Test
{
    void SetUp()
    {
        mock_server_tool = std::make_shared<mt::MockServerSurfaceCounter>();
        mock_server = std::make_shared<mt::TestServer>("./test_socket", mock_server_tool);
 
        ::testing::Mock::VerifyAndClearExpectations(mock_server->factory.get());
        EXPECT_CALL(*mock_server->factory, make_ipc_server()).Times(1);

        mock_server->comm.start();

        mock_client = std::make_shared<mt::TestClient>("./test_socket");
        mock_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        mock_server->comm.stop();
    }

    std::shared_ptr<mt::TestClient> mock_client;
    std::shared_ptr<mt::MockServerSurfaceCounter> mock_server_tool;

    std::shared_ptr<mt::TestServer> mock_server;
};

TEST_F(ProtobufAsioCommunicatorCounter, surface_count_is_zero_after_connection)
{
    using namespace testing;
    EXPECT_CALL(*mock_client, connect_done()).Times(AtLeast(0));

    mock_client->display_server.connect(
        0,
        &mock_client->connect_parameters,
        &mock_client->connection,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::connect_done));
    mock_client->wait_for_connect_done();

    mock_server_tool->expect_surface_count(0);
}

TEST_F(ProtobufAsioCommunicatorCounter, connection_results_in_a_callback)
{
    using namespace testing;
    EXPECT_CALL(*mock_client, create_surface_done()).Times(AtLeast(0));

    mock_client->display_server.create_surface(
        0,
        &mock_client->surface_parameters,
        &mock_client->surface,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));

    mock_client->wait_for_create_surface();

    mock_server_tool->expect_surface_count(1);
}

TEST_F(ProtobufAsioCommunicatorCounter,
       each_create_surface_results_in_a_new_surface_being_created)
{
    int const surface_count{5};

    EXPECT_CALL(*mock_client, create_surface_done()).Times(surface_count);

    for (int i = 0; i != surface_count; ++i)
    {
        mock_client->display_server.create_surface(
            0,
            &mock_client->surface_parameters,
            &mock_client->surface,
            google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));
    
        /* sync on each create */
        mock_client->wait_for_create_surface();
    }

    mock_server_tool->expect_surface_count(surface_count);
}

TEST_F(ProtobufAsioCommunicatorCounter,
       each_create_surface_results_in_a_new_surface_being_created_asynchronously)
{
    int const surface_count{5};

    EXPECT_CALL(*mock_client, create_surface_done()).Times(surface_count);

    for (int i = 0; i != surface_count; ++i)
    {
        mock_client->display_server.create_surface(
            0,
            &mock_client->surface_parameters,
            &mock_client->surface,
            google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));

        /* no sync before calling next create */
    }

    mock_server_tool->expect_surface_count(surface_count);
    mock_client->wait_for_surface_count(surface_count);
}

/* start Multi test cases */
struct ProtobufAsioMultiClientCommunicator : public ::testing::Test
{
    static int const number_of_clients = 10;

    void SetUp()
    {
        using namespace testing;

        mock_server_tool = std::make_shared<mt::MockServerSurfaceCounter>();
        mock_server = std::make_shared<mt::TestServer>("./test_socket", mock_server_tool);
        ::testing::Mock::VerifyAndClearExpectations(mock_server->factory.get());
        EXPECT_CALL(*mock_server->factory, make_ipc_server()).Times(AtLeast(0));

        mock_server->comm.start();

        for(int i=0; i<number_of_clients; i++)
        {
            auto client_tmp = std::make_shared<mt::TestClient>("./test_socket"); 
            mock_clients.push_back(client_tmp);
        }
    }

    void TearDown()
    {
        mock_server->comm.stop();
    }

    std::vector<std::shared_ptr<mt::TestClient>> mock_clients;
    std::shared_ptr<mt::MockServerSurfaceCounter> mock_server_tool;

    std::shared_ptr<mt::TestServer> mock_server;
};

TEST_F(ProtobufAsioMultiClientCommunicator,
       multiple_clients_can_connect_create_surface_and_disconnect)
{
    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*mock_clients[i], create_surface_done()).Times(1);
        mock_clients[i]->display_server.create_surface(
            0,
            &mock_clients[i]->surface_parameters,
            &mock_clients[i]->surface,
            google::protobuf::NewCallback(mock_clients[i].get(), &mt::TestClient::create_surface_done));
        mock_clients[i]->wait_for_create_surface();
    }

    mock_server_tool->expect_surface_count(number_of_clients);

    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*mock_clients[i], disconnect_done()).Times(1);
        mock_clients[i]->display_server.disconnect(
            0,
            &mock_clients[i]->ignored,
            &mock_clients[i]->ignored,
            google::protobuf::NewCallback(mock_clients[i].get(), &mt::TestClient::disconnect_done));
        mock_clients[i]->wait_for_disconnect_done();
    }

    mock_server_tool->expect_surface_count(number_of_clients);
}

TEST_F(ProtobufAsioMultiClientCommunicator,
       multiple_clients_can_connect_and_disconnect_asynchronously)
{
    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*mock_clients[i], create_surface_done()).Times(1);
        mock_clients[i]->display_server.create_surface(
            0,
            &mock_clients[i]->surface_parameters,
            &mock_clients[i]->surface,
            google::protobuf::NewCallback(mock_clients[i].get(), &mt::TestClient::create_surface_done));
    }

    for (int i = 0; i != number_of_clients; ++i)
    {
        mock_clients[i]->wait_for_create_surface();
    }

    mock_server_tool->expect_surface_count(number_of_clients);

    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*mock_clients[i], disconnect_done()).Times(1);
        mock_clients[i]->display_server.disconnect(
            0,
            &mock_clients[i]->ignored,
            &mock_clients[i]->ignored,
            google::protobuf::NewCallback(mock_clients[i].get(), &mt::TestClient::disconnect_done));
    }

    for (int i = 0; i != number_of_clients; ++i)
    {
        mock_clients[i]->wait_for_disconnect_done();
    }

    mock_server_tool->expect_surface_count(number_of_clients);
}
}
