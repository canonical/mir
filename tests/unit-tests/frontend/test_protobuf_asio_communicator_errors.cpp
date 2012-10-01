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

#include "mir_test/stub_server_error.h"
#include "mir_test/mock_ipc_factory.h"
#include "mir_test/test_client.h"

namespace mt = mir::test;

struct TestErrorServer
{
    TestErrorServer(std::string socket_name) :
        factory(std::make_shared<mt::MockIpcFactory>(stub_services)),
        comm(socket_name, factory)
    {
    }

    // "Server" side
    mt::ErrorServer stub_services;
    std::shared_ptr<mt::MockIpcFactory> factory;
    mf::ProtobufAsioCommunicator comm;
};

struct ProtobufErrorTestFixture : public ::testing::Test
{
    void SetUp()
    {
        server = std::make_shared<TestErrorServer>("./test_error_fixture");
        client = std::make_shared<mt::TestClient>("./test_error_fixture");

        ::testing::Mock::VerifyAndClearExpectations(server->factory.get());
        EXPECT_CALL(*server->factory, make_ipc_server()).Times(1);
        server->comm.start();
    }

    void TearDown()
    {
        server->comm.stop();
    }

    std::shared_ptr<TestErrorServer> server;
    std::shared_ptr<mt::TestClient>  client;
};

TEST_F(ProtobufErrorTestFixture, connect_exception)
{
    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    EXPECT_CALL(*client, connect_done()).Times(1);

    mir::protobuf::Connection result;
    client->display_server.connect(
        0,
        &client->connect_parameters,
        &result,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::connect_done));

    client->wait_for_connect_done();

    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(server->stub_services.test_exception_text, result.error());
}

TEST_F(ProtobufErrorTestFixture, create_surface_exception)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_TRUE(client->surface.has_error());
    EXPECT_EQ(server->stub_services.test_exception_text, client->surface.error());
}
