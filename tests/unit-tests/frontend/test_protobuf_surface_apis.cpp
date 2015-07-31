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

#include "mir/frontend/connector.h"
#include "src/server/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"

#include "mir/test/doubles/stub_ipc_factory.h"
#include "mir/test/stub_server_tool.h"
#include "mir/test/test_protobuf_client.h"
#include "mir/test/test_protobuf_server.h"
#include "mir_test_framework/testing_server_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <memory>
#include <string>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace mir
{
namespace test
{

struct StubServerSurfaceCounter : public StubServerTool
{
    int surface_count;

    StubServerSurfaceCounter() : surface_count(0)
    {
    }

    void create_surface(
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done) override
    {
        {
            std::unique_lock<std::mutex> lock(guard);
            ++surface_count;
        }
        StubServerTool::create_surface(request, response, done);
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

struct ProtobufSurfaceCounter : public ::testing::Test
{
    void SetUp()
    {
        stub_server_tool = std::make_shared<mt::StubServerSurfaceCounter>();
        stub_server = std::make_shared<mt::TestProtobufServer>(mtf::test_socket_file(), stub_server_tool);

        stub_server->comm->start();

        stub_client = std::make_shared<mt::TestProtobufClient>(mtf::test_socket_file(), 100);
        stub_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        stub_server.reset();
    }

    std::shared_ptr<mt::TestProtobufClient> stub_client;
    std::shared_ptr<mt::StubServerSurfaceCounter> stub_server_tool;

    std::shared_ptr<mt::TestProtobufServer> stub_server;
};

TEST_F(ProtobufSurfaceCounter, server_creates_surface_on_create_surface_call)
{
    EXPECT_CALL(*stub_client, create_surface_done()).Times(testing::AtLeast(1));

    stub_client->display_server.create_surface(
        &stub_client->surface_parameters,
        &stub_client->surface,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));
    stub_client->wait_for_create_surface();

    stub_server_tool->expect_surface_count(1);
}

TEST_F(ProtobufSurfaceCounter, surface_count_is_zero_after_connection)
{
    using namespace testing;
    EXPECT_CALL(*stub_client, connect_done()).Times(AtLeast(0));

    stub_client->display_server.connect(
        &stub_client->connect_parameters,
        &stub_client->connection,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::connect_done));
    stub_client->wait_for_connect_done();

    stub_server_tool->expect_surface_count(0);
}

TEST_F(ProtobufSurfaceCounter,
       each_create_surface_results_in_a_new_surface_being_created)
{
    int const surface_count{5};

    EXPECT_CALL(*stub_client, create_surface_done()).Times(surface_count);

    for (int i = 0; i != surface_count; ++i)
    {
        stub_client->display_server.create_surface(
            &stub_client->surface_parameters,
            &stub_client->surface,
            google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));

        stub_client->wait_for_create_surface();
    }

    stub_server_tool->expect_surface_count(surface_count);
}

TEST_F(ProtobufSurfaceCounter,
       each_create_surface_results_in_a_new_surface_being_created_asynchronously)
{
    int const surface_count{5};

    EXPECT_CALL(*stub_client, create_surface_done()).Times(surface_count);

    for (int i = 0; i != surface_count; ++i)
    {
        stub_client->display_server.create_surface(
            &stub_client->surface_parameters,
            &stub_client->surface,
            google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));
    }

    stub_server_tool->expect_surface_count(surface_count);
    stub_client->wait_for_surface_count(surface_count);
}


/* start Multi test cases */
struct ProtobufSocketMultiClientCommunicator : public ::testing::Test
{
    static int const number_of_clients = 10;

    void SetUp()
    {
        using namespace testing;

        stub_server_tool = std::make_shared<mt::StubServerSurfaceCounter>();
        stub_server = std::make_shared<mt::TestProtobufServer>(mtf::test_socket_file(), stub_server_tool);
        stub_server->comm->start();

        for(int i=0; i<number_of_clients; i++)
        {
            auto client_tmp = std::make_shared<mt::TestProtobufClient>(mtf::test_socket_file(), 100);
            clients.push_back(client_tmp);
        }
    }

    void TearDown()
    {
        clients.clear();
        stub_server.reset();
        stub_server_tool.reset();
    }

    std::vector<std::shared_ptr<mt::TestProtobufClient>> clients;
    std::shared_ptr<mt::StubServerSurfaceCounter> stub_server_tool;

    std::shared_ptr<mt::TestProtobufServer> stub_server;
};

TEST_F(ProtobufSocketMultiClientCommunicator,
       multiple_clients_can_connect_create_surface_and_disconnect)
{
    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*clients[i], create_surface_done()).Times(1);
        clients[i]->display_server.create_surface(
            &clients[i]->surface_parameters,
            &clients[i]->surface,
            google::protobuf::NewCallback(clients[i].get(), &mt::TestProtobufClient::create_surface_done));
        clients[i]->wait_for_create_surface();
    }

    stub_server_tool->expect_surface_count(number_of_clients);

    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*clients[i], disconnect_done()).Times(1);
        clients[i]->display_server.disconnect(
            &clients[i]->ignored,
            &clients[i]->ignored,
            google::protobuf::NewCallback(clients[i].get(), &mt::TestProtobufClient::disconnect_done));
        clients[i]->wait_for_disconnect_done();
    }

    stub_server_tool->expect_surface_count(number_of_clients);
}

TEST_F(ProtobufSocketMultiClientCommunicator,
       multiple_clients_can_connect_and_disconnect_asynchronously)
{
    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*clients[i], create_surface_done()).Times(1);
        clients[i]->display_server.create_surface(
            &clients[i]->surface_parameters,
            &clients[i]->surface,
            google::protobuf::NewCallback(clients[i].get(), &mt::TestProtobufClient::create_surface_done));
    }

    for (int i = 0; i != number_of_clients; ++i)
    {
        clients[i]->wait_for_create_surface();
    }

    stub_server_tool->expect_surface_count(number_of_clients);

    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*clients[i], disconnect_done()).Times(1);
        clients[i]->display_server.disconnect(
            &clients[i]->ignored,
            &clients[i]->ignored,
            google::protobuf::NewCallback(clients[i].get(), &mt::TestProtobufClient::disconnect_done));
    }

    for (int i = 0; i != number_of_clients; ++i)
    {
        clients[i]->wait_for_disconnect_done();
    }

    stub_server_tool->expect_surface_count(number_of_clients);
}
}
