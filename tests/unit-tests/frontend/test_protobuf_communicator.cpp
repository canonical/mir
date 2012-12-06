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

#include "src/frontend/protobuf_socket_communicator.h"
#include "mir/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"

#include "mir_test/mock_ipc_factory.h"
#include "mir_test/mock_logger.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/test_protobuf_client.h"
#include "mir_test/test_protobuf_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <memory>
#include <string>

namespace mf = mir::frontend;
namespace mt = mir::test;

namespace mir
{
struct ProtobufCommunicator : public ::testing::Test
{
    void SetUp()
    {
//        auto const pid = fork();
//
//        if (pid)
//        {
//            std::cout << "DEBUG (forked) pid=" << pid << std::endl;
            stub_server_tool = std::make_shared<mt::StubServerTool>();
            stub_server = std::make_shared<mt::TestProtobufServer>("./test_socket", stub_server_tool);

            stub_server->comm->start();

            ::testing::Mock::VerifyAndClearExpectations(stub_server->factory.get());
    //        EXPECT_CALL(*stub_server->factory, make_ipc_server()).Times(1);

//            std::cout << "DEBUG (sleep) pid=" << pid << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
//            std::cout << "DEBUG (exit) pid=" << pid << std::endl;
//            stub_server.reset();
//            stub_server_tool.reset();
//            exit(0);
//        }
//        else
//        {
//            std::cout << "DEBUG (forked) pid=" << pid << std::endl;
            client = std::make_shared<mt::TestProtobufClient>("./test_socket", 100);
    //        client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
//            std::cout << "DEBUG (continue) pid=" << pid << std::endl;
//        }
    }

    void TearDown()
    {
        client.reset();
        stub_server.reset();
        stub_server_tool.reset();
    }

    std::shared_ptr<mt::TestProtobufClient> client;
    std::shared_ptr<mt::StubServerTool> stub_server_tool;
private:
    std::shared_ptr<mt::TestProtobufServer> stub_server;
};

TEST_F(ProtobufCommunicator, create_surface_results_in_a_callback)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();
}

TEST_F(ProtobufCommunicator, connection_sets_app_name)
{
    EXPECT_CALL(*client, connect_done()).Times(1);

    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    client->display_server.connect(
        0,
        &client->connect_parameters,
        &client->connection,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::connect_done));

    client->wait_for_connect_done();

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->app_name);
}

TEST_F(ProtobufCommunicator, create_surface_sets_surface_name)
{
    EXPECT_CALL(*client, connect_done()).Times(1);
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    client->display_server.connect(
        0,
        &client->connect_parameters,
        &client->connection,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::connect_done));

    client->wait_for_connect_done();

    client->surface_parameters.set_surface_name(__PRETTY_FUNCTION__);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->surface_name);
}


TEST_F(ProtobufCommunicator,
        create_surface_results_in_a_surface_being_created)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();
}

TEST_F(ProtobufCommunicator,
       double_disconnection_attempt_has_no_effect)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);
    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_CALL(*client, disconnect_done()).Times(1);
    client->display_server.disconnect(
        0,
        &client->ignored,
        &client->ignored,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::disconnect_done));

    client->wait_for_disconnect_done();

    EXPECT_CALL(*client->logger, error()).Times(testing::AtLeast(1));

    // We don't expect this to be called, so it can't auto destruct
    std::unique_ptr<google::protobuf::Closure> new_callback(google::protobuf::NewPermanentCallback(client.get(), &mt::TestProtobufClient::disconnect_done));
    client->display_server.disconnect(0, &client->ignored, &client->ignored, new_callback.get());
    client->wait_for_disconnect_done();
}

TEST_F(ProtobufCommunicator,
       getting_and_advancing_buffers)
{
    EXPECT_CALL(*client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*client, disconnect_done()).Times(testing::AtLeast(0));

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_TRUE(client->surface.has_buffer());
    EXPECT_CALL(*client, next_buffer_done()).Times(8);

    for (int i = 0; i != 8; ++i)
    {
        client->display_server.next_buffer(
            0,
            &client->surface.id(),
            client->surface.mutable_buffer(),
            google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::next_buffer_done));

        client->wait_for_next_buffer();
        EXPECT_TRUE(client->surface.has_buffer());
    }

    client->display_server.disconnect(
        0,
        &client->ignored,
        &client->ignored,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::disconnect_done));

    client->wait_for_disconnect_done();
}

TEST_F(ProtobufCommunicator,
       connect_create_surface_then_disconnect_a_session)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);
    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_CALL(*client, disconnect_done()).Times(1);
    client->display_server.disconnect(
        0,
        &client->ignored,
        &client->ignored,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::disconnect_done));

    client->wait_for_disconnect_done();
}
}
