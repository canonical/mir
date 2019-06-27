/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "mir/events/event_private.h"

#include "src/server/report/null_report_factory.h"
#include "src/server/frontend/resource_cache.h"
#include "src/server/frontend/published_socket_connector.h"

#include "mir/frontend/protobuf_connection_creator.h"
#include "mir/frontend/connector_report.h"

#include "mir_protobuf.pb.h"

#include "mir/test/stub_server_tool.h"
#include "mir/test/test_protobuf_client.h"
#include "mir/test/test_protobuf_server.h"
#include "mir/test/signal_actions.h"
#include "mir/test/doubles/stub_ipc_factory.h"
#include "mir/test/doubles/stub_session_authorizer.h"
#include "mir/test/doubles/mock_rpc_report.h"
#include "mir/test/doubles/stub_ipc_factory.h"
#include "mir_test_framework/testing_server_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;
namespace mtf = mir_test_framework;

namespace
{
const int timeout_ms = 60 * 1000;

class MockConnectorReport : public mf::ConnectorReport
{
public:
    MockConnectorReport()
    {
        using namespace testing;
        EXPECT_CALL(*this, thread_start()).Times(AnyNumber());
        EXPECT_CALL(*this, thread_end()).Times(AnyNumber());
        EXPECT_CALL(*this, creating_session_for(_)).Times(AnyNumber());
        EXPECT_CALL(*this, creating_socket_pair(_, _)).Times(AnyNumber());
        EXPECT_CALL(*this, listening_on(_)).Times(AnyNumber());
        EXPECT_CALL(*this, error(_)).Times(AnyNumber());
    }

    ~MockConnectorReport() noexcept {}

    MOCK_METHOD0(thread_start, void ());
    MOCK_METHOD0(thread_end, void());

    MOCK_METHOD1(creating_session_for, void(int socket_handle));
    MOCK_METHOD2(creating_socket_pair, void(int server_handle, int client_handle));

    MOCK_METHOD1(listening_on, void(std::string const& endpoint));

    MOCK_METHOD1(error, void (std::exception const& error));

    void warning(std::string const& /*error*/) /*override*/ {}
};
}

struct PublishedSocketConnector : public ::testing::Test
{
    static std::string const test_socket;
    static void SetUpTestCase()
    {
        remove(test_socket.c_str());
    }

    void SetUp()
    {
        communicator_report = std::make_shared<MockConnectorReport>();
        stub_server_tool = std::make_shared<mt::StubServerTool>();
        stub_server = std::make_shared<mt::TestProtobufServer>(
            test_socket,
            stub_server_tool,
            communicator_report);

        stub_server->comm->start();
        client = std::make_shared<mt::TestProtobufClient>(test_socket, timeout_ms);
        client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        client.reset();

        stub_server->comm->stop();
        stub_server.reset();
        stub_server_tool.reset();
        communicator_report.reset();
    }

    static void TearDownTestCase()
    {
    }

    std::shared_ptr<mt::TestProtobufClient> client;
    static std::shared_ptr<MockConnectorReport> communicator_report;
    static std::shared_ptr<mt::StubServerTool> stub_server_tool;
    static std::shared_ptr<mt::TestProtobufServer> stub_server;
};

std::string const PublishedSocketConnector::test_socket = mtf::test_socket_file();

std::shared_ptr<mt::StubServerTool> PublishedSocketConnector::stub_server_tool;
std::shared_ptr<MockConnectorReport> PublishedSocketConnector::communicator_report;
std::shared_ptr<mt::TestProtobufServer> PublishedSocketConnector::stub_server;

TEST_F(PublishedSocketConnector, create_surface_results_in_a_callback)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->create_surface();
    client->wait_for_create_surface();
}

TEST_F(PublishedSocketConnector, connection_sets_app_name)
{
    EXPECT_CALL(*client, connect_done()).Times(1);

    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    client->connect();
    client->wait_for_connect_done();

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->application_name());
}

TEST_F(PublishedSocketConnector, create_surface_sets_surface_name)
{
    EXPECT_CALL(*client, connect_done()).Times(1);
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    client->connect();
    client->wait_for_connect_done();

    client->surface_parameters.set_surface_name(__PRETTY_FUNCTION__);

    client->create_surface();
    client->wait_for_create_surface();

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->surface_name());
}

TEST_F(PublishedSocketConnector,
        create_surface_results_in_a_surface_being_created)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->create_surface();
    client->wait_for_create_surface();
}

namespace
{

MATCHER_P(InvocationMethodEq, name, "")
{
    return arg.method_name() == name;
}

}

TEST_F(PublishedSocketConnector, double_disconnection_does_not_break)
{
    using namespace testing;

    EXPECT_CALL(*client, create_surface_done()).Times(1);
    client->create_surface();
    client->wait_for_create_surface();

    EXPECT_CALL(*client, disconnect_done()).Times(1);
    client->disconnect();
    client->wait_for_disconnect_done();

    Mock::VerifyAndClearExpectations(client.get());

    // There is a race between the server closing the socket and
    // the client sending the second "disconnect".
    // Usually[*] the server wins and the "disconnect" fails resulting
    // in an invocation_failed() report and an exception.
    // Occasionally, the client wins and the message is sent to dying socket.
    // [*] ON my desktop under valgrind "Usually" is ~49 times out of 50

    EXPECT_CALL(*client->rpc_report, invocation_failed(InvocationMethodEq("disconnect"),_)).Times(AtMost(1));
    EXPECT_CALL(*client, disconnect_done()).Times(1);

    try
    {
        client->disconnect();
        // the write beat the socket closing
    }
    catch (std::runtime_error const& x)
    {
        // the socket closing beat the write
        EXPECT_THAT(x.what(), HasSubstr("Failed to send message to server:"));
    }

    client->wait_for_disconnect_done();
}

TEST_F(PublishedSocketConnector, getting_and_submitting_buffers)
{
    EXPECT_CALL(*client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*client, disconnect_done()).Times(testing::AtLeast(0));

    client->create_surface();
    client->wait_for_create_surface();

    EXPECT_TRUE(client->surface.has_buffer());
    EXPECT_CALL(*client, submit_buffer_done()).Times(8);

    for (int i = 0; i != 8; ++i)
    {
        client->submit_buffer();
        client->wait_for_submit_buffer();
        EXPECT_TRUE(client->surface.has_buffer());
    }

    client->disconnect();
    client->wait_for_disconnect_done();
}

TEST_F(PublishedSocketConnector,
       connect_create_surface_then_disconnect_a_session)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);
    client->create_surface();
    client->wait_for_create_surface();

    EXPECT_CALL(*client, disconnect_done()).Times(1);
    client->disconnect();
    client->wait_for_disconnect_done();
}

TEST_F(PublishedSocketConnector, disorderly_disconnection_handled)
{
    using namespace testing;

    EXPECT_CALL(*client, create_surface_done()).Times(AnyNumber());
    client->create_surface();
    client->wait_for_create_surface();

    mt::Signal error_reported;

    ON_CALL(*communicator_report, error(_)).WillByDefault(WakeUp(&error_reported));

    EXPECT_CALL(*communicator_report, error(_)).Times(1);

    client.reset();

    error_reported.wait_for(std::chrono::seconds{1});
    EXPECT_TRUE(error_reported.raised());
}

TEST_F(PublishedSocketConnector, configure_display)
{
    EXPECT_CALL(*client, display_configure_done())
        .Times(1);

    client->configure_display();
    client->wait_for_configure_display_done();
}

TEST_F(PublishedSocketConnector, connection_using_socket_fd)
{
    int const submit_buffer_calls{8};
    char buffer[128] = {0};
    sprintf(buffer, "fd://%d", stub_server->comm->client_socket_fd());
    auto client = std::make_shared<mt::TestProtobufClient>(buffer, timeout_ms);
    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);

    EXPECT_CALL(*client, connect_done()).Times(1);
    client->connect();
    client->wait_for_connect_done();

    EXPECT_CALL(*client, create_surface_done()).Times(testing::AtLeast(0));
    EXPECT_CALL(*client, disconnect_done()).Times(testing::AtLeast(0));

    client->create_surface();
    client->wait_for_create_surface();

    EXPECT_TRUE(client->surface.has_buffer());
    EXPECT_CALL(*client, submit_buffer_done()).Times(submit_buffer_calls);

    for (int i = 0; i != submit_buffer_calls; ++i)
    {
        client->submit_buffer();
        client->wait_for_submit_buffer();
        EXPECT_TRUE(client->surface.has_buffer());
    }

    client->disconnect();
    client->wait_for_disconnect_done();

    EXPECT_EQ(__PRETTY_FUNCTION__, stub_server_tool->app_name);
}
