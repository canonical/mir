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
struct ProtobufAsioCommunicatorBasic : public ::testing::Test
{
    void SetUp()
    {
        mock_server_tool = std::make_shared<mt::MockServerTool>();
        mock_server = std::make_shared<mt::TestServer>("./test_socket", mock_server_tool);
 
        ::testing::Mock::VerifyAndClearExpectations(mock_server->factory.get());
        EXPECT_CALL(*mock_server->factory, make_ipc_server()).Times(1);

        mock_server->comm.start();

        mock_client = std::make_shared<mt::TestClient>("./test_socket");
    }

    void TearDown()
    {
        mock_server->comm.stop();
    }

    std::shared_ptr<mt::TestClient> mock_client;
    std::shared_ptr<mt::MockServerTool> mock_server_tool;

private:
    std::shared_ptr<mt::TestServer> mock_server;
};

TEST_F(ProtobufAsioCommunicatorBasic,surface_creation_results_in_a_callback)
{
    EXPECT_CALL(*mock_client, create_surface_done()).Times(1);

    mock_client->display_server.create_surface(
        0,
        &mock_client->surface_parameters,
        &mock_client->surface,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));
    mock_client->wait_for_create_surface();
}


TEST_F(ProtobufAsioCommunicatorBasic, connection_sets_app_name_from_parameter)
{
    using namespace testing;
    EXPECT_CALL(*mock_client, connect_done()).Times(AtLeast(0));

    mock_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    mock_client->display_server.connect(
        0,
        &mock_client->connect_parameters,
        &mock_client->connection,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::connect_done));
    mock_client->wait_for_connect_done();

    EXPECT_EQ(__PRETTY_FUNCTION__, mock_server_tool->app_name);
}

TEST_F(ProtobufAsioCommunicatorBasic, create_surface_sets_surface_name)
{
    using namespace testing;
    EXPECT_CALL(*mock_client, connect_done()).Times(AtLeast(0));
    EXPECT_CALL(*mock_client, create_surface_done()).Times(AtLeast(0));

    mock_client->display_server.connect(
        0,
        &mock_client->connect_parameters,
        &mock_client->connection,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::connect_done));
    mock_client->wait_for_connect_done();

    mock_client->surface_parameters.set_surface_name(__PRETTY_FUNCTION__);
    mock_client->display_server.create_surface(
        0,
        &mock_client->surface_parameters,
        &mock_client->surface,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));
    mock_client->wait_for_create_surface();

    EXPECT_EQ(__PRETTY_FUNCTION__, mock_server_tool->surface_name);
}

TEST_F(ProtobufAsioCommunicatorBasic,
        create_surface_results_in_callback)
{
    EXPECT_CALL(*mock_client, create_surface_done()).Times(1);

    mock_client->display_server.create_surface(
        0,
        &mock_client->surface_parameters,
        &mock_client->surface,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));

    mock_client->wait_for_create_surface();
}

TEST_F(ProtobufAsioCommunicatorBasic,
       double_disconnection_attempt_has_no_effect)
{
    // We don't expect this to be called, so it can't auto destruct
    std::unique_ptr<google::protobuf::Closure> new_callback(
                            google::protobuf::NewPermanentCallback(
                                mock_client.get(),
                                &mt::TestClient::disconnect_done));
    EXPECT_CALL(*mock_client, disconnect_done()).Times(1);
    EXPECT_CALL(*mock_client->logger, error()).Times(testing::AtLeast(1));

    mock_client->display_server.disconnect(
        0,
        &mock_client->ignored,
        &mock_client->ignored,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::disconnect_done));
    mock_client->wait_for_disconnect_done();

    mock_client->display_server.disconnect(
        0,
        &mock_client->ignored,
        &mock_client->ignored,
        new_callback.get());
    mock_client->wait_for_disconnect_done();
}

TEST_F(ProtobufAsioCommunicatorBasic, client_has_buffer_on_creation)
{
    EXPECT_CALL(*mock_client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*mock_client, disconnect_done()).Times(testing::AtLeast(0));

    mock_client->display_server.create_surface(
        0,
        &mock_client->surface_parameters,
        &mock_client->surface,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));
    mock_client->wait_for_create_surface();

    EXPECT_TRUE(mock_client->surface.has_buffer());
}

TEST_F(ProtobufAsioCommunicatorBasic, buffer_advances_on_each_call_to_next_buffer)
{
    EXPECT_CALL(*mock_client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*mock_client, disconnect_done()).Times(testing::AtLeast(0));

    mock_client->display_server.create_surface(
        0,
        &mock_client->surface_parameters,
        &mock_client->surface,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::create_surface_done));
    mock_client->wait_for_create_surface();

    EXPECT_CALL(*mock_client, next_buffer_done()).Times(8);
    for (int i = 0; i != 8; ++i)
    {
        mock_client->display_server.next_buffer(
            0,
            &mock_client->surface.id(),
            mock_client->surface.mutable_buffer(),
            google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::next_buffer_done));

        mock_client->wait_for_next_buffer();
        EXPECT_TRUE(mock_client->surface.has_buffer());
    }
}

#if 0
// what are we testing? the callback of disconnect? 
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
#endif
}
