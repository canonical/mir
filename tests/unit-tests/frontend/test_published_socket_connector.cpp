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
#include "mir/frontend/connector_report.h"
#include "mir/frontend/resource_cache.h"
#include "src/server/frontend/published_socket_connector.h"
#include "src/server/frontend/protobuf_session_creator.h"

#include "mir_protobuf.pb.h"

#include "mir_test_doubles/stub_ipc_factory.h"
#include "mir_test_doubles/stub_session_authorizer.h"
#include "mir_test_doubles/mock_rpc_report.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/test_protobuf_client.h"
#include "mir_test/test_protobuf_server.h"
#include "mir_test_doubles/stub_ipc_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <memory>
#include <string>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
class MockCommunicatorReport : public mf::ConnectorReport
{
public:

    ~MockCommunicatorReport() noexcept {}

    MOCK_METHOD1(error, void (std::exception const& error));
};
}

struct PublishedSocketConnector : public ::testing::Test
{
    static void SetUpTestCase()
    {
    }

    void SetUp()
    {
        communicator_report = std::make_shared<MockCommunicatorReport>();
        stub_server_tool = std::make_shared<mt::StubServerTool>();
        stub_server = std::make_shared<mt::TestProtobufServer>(
            "./test_socket",
            stub_server_tool,
            communicator_report);

        using namespace testing;
        EXPECT_CALL(*communicator_report, error(_)).Times(AnyNumber());
        stub_server->comm->start();
        client = std::make_shared<mt::TestProtobufClient>("./test_socket", 100);
        client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        stub_server->comm->stop();
        testing::Mock::VerifyAndClearExpectations(communicator_report.get());
        client.reset();

        stub_server.reset();
        stub_server_tool.reset();
        communicator_report.reset();
    }

    static void TearDownTestCase()
    {
    }

    std::shared_ptr<mt::TestProtobufClient> client;
    static std::shared_ptr<MockCommunicatorReport> communicator_report;
    static std::shared_ptr<mt::StubServerTool> stub_server_tool;
    static std::shared_ptr<mt::TestProtobufServer> stub_server;
};

std::shared_ptr<mt::StubServerTool> PublishedSocketConnector::stub_server_tool;
std::shared_ptr<MockCommunicatorReport> PublishedSocketConnector::communicator_report;
std::shared_ptr<mt::TestProtobufServer> PublishedSocketConnector::stub_server;

TEST_F(PublishedSocketConnector, create_surface_results_in_a_callback)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();
}

TEST_F(PublishedSocketConnector, connection_sets_app_name)
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

TEST_F(PublishedSocketConnector, create_surface_sets_surface_name)
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


TEST_F(PublishedSocketConnector,
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

namespace
{

MATCHER_P(InvocationMethodEq, name, "")
{
    return arg.method_name() == name;
}

}

TEST_F(PublishedSocketConnector, double_disconnection_attempt_throws_exception)
{
    using namespace testing;

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

    Mock::VerifyAndClearExpectations(client.get());

    EXPECT_CALL(*client->rpc_report, invocation_failed(InvocationMethodEq("disconnect"),_));
    EXPECT_CALL(*client, disconnect_done()).Times(0);

    EXPECT_THROW({
    // We don't know if this will be called, so it can't auto destruct
    std::unique_ptr<google::protobuf::Closure> new_callback(google::protobuf::NewPermanentCallback(client.get(), &mt::TestProtobufClient::disconnect_done));
    client->display_server.disconnect(0, &client->ignored, &client->ignored, new_callback.get());
    client->wait_for_disconnect_done();
    }, std::runtime_error);
}

TEST_F(PublishedSocketConnector, getting_and_advancing_buffers)
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

TEST_F(PublishedSocketConnector,
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

TEST_F(PublishedSocketConnector, drm_auth_magic_is_processed_by_the_server)
{
    mir::protobuf::DRMMagic magic;
    mir::protobuf::DRMAuthMagicStatus status;
    magic.set_magic(0x10111213);

    EXPECT_CALL(*client, drm_auth_magic_done()).Times(1);

    client->display_server.drm_auth_magic(
        0,
        &magic,
        &status,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::drm_auth_magic_done));

    client->wait_for_drm_auth_magic_done();

    EXPECT_EQ(magic.magic(), stub_server_tool->drm_magic);
}

namespace
{

class MockForceRequests
{
public:
    MOCK_METHOD0(force_requests_to_complete, void());
};

}

TEST_F(PublishedSocketConnector, forces_requests_to_complete_when_stopping)
{
    MockForceRequests mock_force_requests;
    auto stub_server_tool = std::make_shared<mt::StubServerTool>();
    auto ipc_factory = std::make_shared<mtd::StubIpcFactory>(*stub_server_tool);

    /* Once for the explicit stop() and once when the communicator is destroyed */
    EXPECT_CALL(mock_force_requests, force_requests_to_complete())
        .Times(2);

    auto comms = std::make_shared<mf::PublishedSocketConnector>(
        "./test_socket1",
        std::make_shared<mf::ProtobufSessionCreator>(ipc_factory, std::make_shared<mtd::StubSessionAuthorizer>()),
        10,
        std::bind(&MockForceRequests::force_requests_to_complete, &mock_force_requests),
        std::make_shared<mf::NullConnectorReport>());

    comms->start();
    comms->stop();
}

TEST_F(PublishedSocketConnector, disorderly_disconnection_handled)
{
    using namespace testing;

    EXPECT_CALL(*client, create_surface_done()).Times(AnyNumber());
    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    std::mutex m;
    std::condition_variable cv;
    bool done = false;

    ON_CALL(*communicator_report, error(_)).WillByDefault(Invoke([&] (std::exception const&)
        {
            std::unique_lock<std::mutex> lock(m);

            done = true;
            cv.notify_all();
        }));

    EXPECT_CALL(*communicator_report, error(_)).Times(1);

    client.reset();

    std::unique_lock<std::mutex> lock(m);

    auto const deadline = std::chrono::system_clock::now() + std::chrono::seconds(1);
    while (!done && cv.wait_until(lock, deadline) != std::cv_status::timeout);
}

TEST_F(PublishedSocketConnector, configure_display)
{
    EXPECT_CALL(*client, display_configure_done())
        .Times(1);

    client->display_server.configure_display(
        0,
        &client->disp_config,
        &client->disp_config_response,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::display_configure_done));

    client->wait_for_configure_display_done();
}

TEST_F(PublishedSocketConnector, connection_using_socket_fd)
{
    int const next_buffer_calls{8};
    char buffer[128] = {0};
    sprintf(buffer, "fd://%d", stub_server->comm->client_socket_fd());
    auto client = std::make_shared<mt::TestProtobufClient>(buffer, 100);
    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    EXPECT_CALL(*client, connect_done()).Times(1);

    client->display_server.connect(
        0,
        &client->connect_parameters,
        &client->connection,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::connect_done));

    EXPECT_CALL(*client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*client, disconnect_done()).Times(testing::AtLeast(0));

    client->display_server.create_surface(
        0,
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    EXPECT_TRUE(client->surface.has_buffer());
    EXPECT_CALL(*client, next_buffer_done()).Times(next_buffer_calls);

    for (int i = 0; i != next_buffer_calls; ++i)
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

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->app_name);
}

