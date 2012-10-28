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
#include "src/client/mir_rpc_channel.h"

#include "mir_test/mock_ipc_factory.h"
#include "mir_test/mock_logger.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/test_protobuf_client.h"
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
struct ProtobufAsioCommunicatorBasic : public ::testing::Test
{
    void SetUp()
    {
        stub_server_tool = std::make_shared<mt::StubServerTool>();
        stub_server = std::make_shared<mt::TestServer>("./test_socket", stub_server_tool);
 
        ::testing::Mock::VerifyAndClearExpectations(stub_server->factory.get());
        EXPECT_CALL(*stub_server->factory, make_ipc_server()).Times(1);

        stub_server->comm.start();

        stub_client = std::make_shared<mt::TestProtobufClient>("./test_socket", 100);
        stub_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        stub_server->comm.stop();
    }

    std::shared_ptr<mt::TestProtobufClient> stub_client;
    std::shared_ptr<mt::StubServerTool> stub_server_tool;
private:
    std::shared_ptr<mt::TestServer> stub_server;
};

TEST_F(ProtobufAsioCommunicatorBasic, create_surface_results_in_a_callback)
{
    EXPECT_CALL(*stub_client, create_surface_done()).Times(1);

    stub_client->display_server.create_surface(
        0,
        &stub_client->surface_parameters,
        &stub_client->surface,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));

    stub_client->wait_for_create_surface();
}

TEST_F(ProtobufAsioCommunicatorBasic, connection_sets_app_name)
{
    EXPECT_CALL(*stub_client, connect_done()).Times(1);

    stub_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    stub_client->display_server.connect(
        0,
        &stub_client->connect_parameters,
        &stub_client->connection,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::connect_done));

    stub_client->wait_for_connect_done();

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->app_name);
}

TEST_F(ProtobufAsioCommunicatorBasic, create_surface_sets_surface_name)
{
    EXPECT_CALL(*stub_client, connect_done()).Times(1);
    EXPECT_CALL(*stub_client, create_surface_done()).Times(1);

    stub_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    stub_client->display_server.connect(
        0,
        &stub_client->connect_parameters,
        &stub_client->connection,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::connect_done));

    stub_client->wait_for_connect_done();

    stub_client->surface_parameters.set_surface_name(__PRETTY_FUNCTION__);

    stub_client->display_server.create_surface(
        0,
        &stub_client->surface_parameters,
        &stub_client->surface,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));

    stub_client->wait_for_create_surface();

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->surface_name);
}


TEST_F(ProtobufAsioCommunicatorBasic,
        create_surface_results_in_a_surface_being_created)
{
    EXPECT_CALL(*stub_client, create_surface_done()).Times(1);

    stub_client->display_server.create_surface(
        0,
        &stub_client->surface_parameters,
        &stub_client->surface,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));

    stub_client->wait_for_create_surface();
}

TEST_F(ProtobufAsioCommunicatorBasic,
       double_disconnection_attempt_has_no_effect)
{
    EXPECT_CALL(*stub_client, create_surface_done()).Times(1);
    stub_client->display_server.create_surface(
        0,
        &stub_client->surface_parameters,
        &stub_client->surface,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));

    stub_client->wait_for_create_surface();

    EXPECT_CALL(*stub_client, disconnect_done()).Times(1);
    stub_client->display_server.disconnect(
        0,
        &stub_client->ignored,
        &stub_client->ignored,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::disconnect_done));

    stub_client->wait_for_disconnect_done();

    EXPECT_CALL(*stub_client->logger, error()).Times(testing::AtLeast(1));

    // We don't expect this to be called, so it can't auto destruct
    std::unique_ptr<google::protobuf::Closure> new_callback(google::protobuf::NewPermanentCallback(stub_client.get(), &mt::TestProtobufClient::disconnect_done));
    stub_client->display_server.disconnect(0, &stub_client->ignored, &stub_client->ignored, new_callback.get());
    stub_client->wait_for_disconnect_done();
}

TEST_F(ProtobufAsioCommunicatorBasic,
       getting_and_advancing_buffers)
{
    EXPECT_CALL(*stub_client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*stub_client, disconnect_done()).Times(testing::AtLeast(0));

    stub_client->display_server.create_surface(
        0,
        &stub_client->surface_parameters,
        &stub_client->surface,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));

    stub_client->wait_for_create_surface();

    EXPECT_TRUE(stub_client->surface.has_buffer());
    EXPECT_CALL(*stub_client, next_buffer_done()).Times(8);

    for (int i = 0; i != 8; ++i)
    {
        stub_client->display_server.next_buffer(
            0,
            &stub_client->surface.id(),
            stub_client->surface.mutable_buffer(),
            google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::next_buffer_done));

        stub_client->wait_for_next_buffer();
        EXPECT_TRUE(stub_client->surface.has_buffer());
    }

    stub_client->display_server.disconnect(
        0,
        &stub_client->ignored,
        &stub_client->ignored,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::disconnect_done));

    stub_client->wait_for_disconnect_done();
}

TEST_F(ProtobufAsioCommunicatorBasic,
       connect_create_surface_then_disconnect_a_session)
{
    EXPECT_CALL(*stub_client, create_surface_done()).Times(1);
    stub_client->display_server.create_surface(
        0,
        &stub_client->surface_parameters,
        &stub_client->surface,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::create_surface_done));

    stub_client->wait_for_create_surface();

    EXPECT_CALL(*stub_client, disconnect_done()).Times(1);
    stub_client->display_server.disconnect(
        0,
        &stub_client->ignored,
        &stub_client->ignored,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::disconnect_done));

    stub_client->wait_for_disconnect_done();
}
}
