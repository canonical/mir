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

struct StubServer : public MockServerTool
{
    static const int file_descriptors = 5;

    int surface_count;
    int file_descriptor[file_descriptors];

    StubServer() : surface_count(0)
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            *i = 0;
    }

    void create_surface(google::protobuf::RpcController* controller,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    { 
        ++surface_count;
        MockServerTool::create_surface(controller, request, response, done);
    }

    void test_file_descriptors(::google::protobuf::RpcController* ,
                         const ::mir::protobuf::Void* ,
                         ::mir::protobuf::Buffer* fds,
                         ::google::protobuf::Closure* done)
    {
        for (int i = 0; i != file_descriptors; ++i)
        {
            static char const test_file_fmt[] = "fd_test_file%d";
            char test_file[sizeof test_file_fmt];
            sprintf(test_file, test_file_fmt, i);
            remove(test_file);
            file_descriptor[i] = open(test_file, O_CREAT, S_IWUSR|S_IRUSR);

            fds->add_fd(file_descriptor[i]);
        }

        done->Run();
    }

    void close_files()
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            close(*i), *i = 0;
    }
};
const int StubServer::file_descriptors;

struct TestServer
{
    TestServer(std::string socket_name) :
        factory(std::make_shared<mt::MockIpcFactory>(stub_services)),
        comm(socket_name, factory)
    {
    }

    void expect_surface_count(int expected_count)
    {
        std::unique_lock<std::mutex> ul(stub_services.guard);
        while (stub_services.surface_count != expected_count)
            stub_services.wait_condition.wait(ul);

        EXPECT_EQ(expected_count, stub_services.surface_count);
    }

    // "Server" side
    mt::StubServer stub_services;
    std::shared_ptr<mt::MockIpcFactory> factory;
    mf::ProtobufAsioCommunicator comm;
};
}

struct ProtobufAsioCommunicatorTestFixture : public ::testing::Test
{
    void SetUp()
    {
        server = std::make_shared<mt::TestServer>("./test_socket");
 
        ::testing::Mock::VerifyAndClearExpectations(server->factory.get());
        EXPECT_CALL(*server->factory, make_ipc_server()).Times(1);

        server->comm.start();

        client = std::make_shared<mt::TestClient>("./test_socket");
    }

    void TearDown()
    {
        server->comm.stop();
    }

    std::shared_ptr<mt::TestClient> client;
    std::shared_ptr<mt::TestServer> server;
};

struct ProtobufAsioMultiClientCommunicatorTestFixture : public ::testing::Test
{
    static int const number_of_clients = 10;

    void SetUp()
    {
        server = std::make_shared<mt::TestServer>("./test_socket");
        ::testing::Mock::VerifyAndClearExpectations(server->factory.get());
        EXPECT_CALL(*server->factory, make_ipc_server()).Times(number_of_clients);

        server->comm.start();

        for(int i=0; i<number_of_clients; i++)
        {
            auto client_tmp = std::make_shared<mt::TestClient>("./test_socket"); 
            client.push_back(client_tmp);
        }
    }

    void TearDown()
    {
        server->comm.stop();
    }

    std::vector<std::shared_ptr<mt::TestClient>> client;
    std::shared_ptr<mt::TestServer> server;
};


TEST_F(ProtobufAsioCommunicatorTestFixture, connection_results_in_a_callback)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

    client->wait_for_create_surface();

    server->expect_surface_count(1);
}

TEST_F(ProtobufAsioCommunicatorTestFixture, connection_sets_app_name)
{
    EXPECT_CALL(*client, connect_done()).Times(1);

    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    client->display_server.connect(
        0,
        &client->connect_parameters,
        &client->connection,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::connect_done));

    client->wait_for_connect_done();

    server->expect_surface_count(0);

    EXPECT_EQ(__PRETTY_FUNCTION__, server->stub_services.app_name);
}

TEST_F(ProtobufAsioCommunicatorTestFixture, create_surface_sets_surface_name)
{
    EXPECT_CALL(*client, connect_done()).Times(1);
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    client->display_server.connect(
        0,
        &client->connect_parameters,
        &client->connection,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::connect_done));

    client->wait_for_connect_done();

    client->surface_parameters.set_surface_name(__PRETTY_FUNCTION__);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_EQ(__PRETTY_FUNCTION__, server->stub_services.surface_name);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
        create_surface_results_in_a_surface_being_created)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

    client->wait_for_create_surface();
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       each_create_surface_results_in_a_new_surface_being_created)
{
    int const surface_count{5};

    EXPECT_CALL(*client, create_surface_done()).Times(surface_count);

    for (int i = 0; i != surface_count; ++i)
    {
        client->display_server.create_surface(
            0,
            &client->surface_parameters,
            &client->surface,
            google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

        client->wait_for_create_surface();
    }

    server->expect_surface_count(surface_count);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       connect_create_surface_then_disconnect_a_session)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);
    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_CALL(*client, disconnect_done()).Times(1);
    client->display_server.disconnect(
        0,
        &client->ignored,
        &client->ignored,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::disconnect_done));

    client->wait_for_disconnect_done();
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       double_disconnection_attempt_has_no_effect)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);
    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_CALL(*client, disconnect_done()).Times(1);
    client->display_server.disconnect(
        0,
        &client->ignored,
        &client->ignored,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::disconnect_done));

    client->wait_for_disconnect_done();

    EXPECT_CALL(*client->logger, error()).Times(testing::AtLeast(1));

    // We don't expect this to be called, so it can't auto destruct
    std::unique_ptr<google::protobuf::Closure> new_callback(google::protobuf::NewPermanentCallback(client.get(), &mt::TestClient::disconnect_done));
    client->display_server.disconnect(0, &client->ignored, &client->ignored, new_callback.get());
    client->wait_for_disconnect_done();
}


TEST_F(ProtobufAsioCommunicatorTestFixture,
       each_create_surface_results_in_a_new_surface_being_created_asynchronously)
{
    int const surface_count{5};

    EXPECT_CALL(*client, create_surface_done()).Times(surface_count);

    for (int i = 0; i != surface_count; ++i)
    {
        client->display_server.create_surface(
            0,
            &client->surface_parameters,
            &client->surface,
            google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));
    }

    server->expect_surface_count(surface_count);
    client->wait_for_surface_count(surface_count);
}

TEST_F(ProtobufAsioCommunicatorTestFixture, test_file_descriptors)
{
    mir::protobuf::Buffer fds;

    client->display_server.test_file_descriptors(0, &client->ignored, &fds,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::tfd_done));

    client->wait_for_tfd_done();

    ASSERT_EQ(server->stub_services.file_descriptors, fds.fd_size());

    for (int i  = 0; i != server->stub_services.file_descriptors; ++i)
    {
        int const fd = fds.fd(i);
        EXPECT_NE(-1, fd);

        for (int j  = 0; j != server->stub_services.file_descriptors; ++j)
        {
            EXPECT_NE(server->stub_services.file_descriptor[j], fd);
        }
    }

    server->stub_services.close_files();
    for (int i  = 0; i != server->stub_services.file_descriptors; ++i)
        close(fds.fd(i));
}


TEST_F(ProtobufAsioCommunicatorTestFixture,
       getting_and_advancing_buffers)
{
    EXPECT_CALL(*client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*client, disconnect_done()).Times(testing::AtLeast(0));

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_TRUE(client->surface.has_buffer());
    EXPECT_CALL(*client, next_buffer_done()).Times(8);

    for (int i = 0; i != 8; ++i)
    {
        client->display_server.next_buffer(
            0,
            &client->surface.id(),
            client->surface.mutable_buffer(),
            google::protobuf::NewCallback(client.get(), &mt::TestClient::next_buffer_done));

        client->wait_for_next_buffer();
        EXPECT_TRUE(client->surface.has_buffer());
    }

    client->display_server.disconnect(
        0,
        &client->ignored,
        &client->ignored,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::disconnect_done));

    client->wait_for_disconnect_done();
}

TEST_F(ProtobufAsioMultiClientCommunicatorTestFixture,
       multiple_clients_can_connect_create_surface_and_disconnect)
{
    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*client[i], create_surface_done()).Times(1);
        client[i]->display_server.create_surface(
            0,
            &client[i]->surface_parameters,
            &client[i]->surface,
            google::protobuf::NewCallback(client[i].get(), &mt::TestClient::create_surface_done));
        client[i]->wait_for_create_surface();
    }

    server->expect_surface_count(number_of_clients);

    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*client[i], disconnect_done()).Times(1);
        client[i]->display_server.disconnect(
            0,
            &client[i]->ignored,
            &client[i]->ignored,
            google::protobuf::NewCallback(client[i].get(), &mt::TestClient::disconnect_done));
        client[i]->wait_for_disconnect_done();
    }

    server->expect_surface_count(number_of_clients);
}

TEST_F(ProtobufAsioMultiClientCommunicatorTestFixture,
       multiple_clients_can_connect_and_disconnect_asynchronously)
{
    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*client[i], create_surface_done()).Times(1);
        client[i]->display_server.create_surface(
            0,
            &client[i]->surface_parameters,
            &client[i]->surface,
            google::protobuf::NewCallback(client[i].get(), &mt::TestClient::create_surface_done));
    }

    for (int i = 0; i != number_of_clients; ++i)
    {
        client[i]->wait_for_create_surface();
    }

    server->expect_surface_count(number_of_clients);

    for (int i = 0; i != number_of_clients; ++i)
    {
        EXPECT_CALL(*client[i], disconnect_done()).Times(1);
        client[i]->display_server.disconnect(
            0,
            &client[i]->ignored,
            &client[i]->ignored,
            google::protobuf::NewCallback(client[i].get(), &mt::TestClient::disconnect_done));
    }

    for (int i = 0; i != number_of_clients; ++i)
    {
        client[i]->wait_for_disconnect_done();
    }

    server->expect_surface_count(number_of_clients);
}

}
