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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/frontend/session_mediator_report.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test/test_protobuf_client.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{
struct MockApplicationMediatorReport : mf::SessionMediatorReport
{
    MockApplicationMediatorReport()
    {
        EXPECT_CALL(*this, session_connect_called(testing::_)).
            Times(testing::AtLeast(0));

        EXPECT_CALL(*this, session_create_surface_called(testing::_)).
            Times(testing::AtLeast(0));

        EXPECT_CALL(*this, session_next_buffer_called(testing::_)).
            Times(testing::AtLeast(0));

        EXPECT_CALL(*this, session_release_surface_called(testing::_)).
            Times(testing::AtLeast(0));

        EXPECT_CALL(*this, session_disconnect_called(testing::_)).
            Times(testing::AtLeast(0));
    }

    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_next_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));

    void session_drm_auth_magic_called(const std::string&) override {};
    void session_configure_surface_called(std::string const&) override {};
    void session_configure_display_called(std::string const&) override {};
    void session_error(const std::string&, const char*, const std::string&) override {};
};

const int rpc_timeout_ms{100000};

typedef BespokeDisplayServerTestFixture ApplicationMediatorReport;
}

TEST_F(ApplicationMediatorReport, session_connect_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport>
        the_application_mediator_report()
        {
            auto result = std::make_shared<MockApplicationMediatorReport>();

            EXPECT_CALL(*result, session_connect_called(testing::_)).
                Times(1);

            return result;
        }
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            mt::TestProtobufClient client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).
                Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(ApplicationMediatorReport, session_create_surface_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport>
        the_application_mediator_report()
        {
            auto result = std::make_shared<MockApplicationMediatorReport>();

            EXPECT_CALL(*result, session_create_surface_called(testing::_)).
                Times(1);

            return result;
        }
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            mt::TestProtobufClient client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).
                Times(testing::AtLeast(0));
            EXPECT_CALL(client, create_surface_done()).
                Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::create_surface_done));
            client.wait_for_create_surface();

        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(ApplicationMediatorReport, session_next_buffer_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport>
        the_application_mediator_report()
        {
            auto result = std::make_shared<MockApplicationMediatorReport>();

            EXPECT_CALL(*result, session_next_buffer_called(testing::_)).
                Times(1);

            return result;
        }
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            mt::TestProtobufClient client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, create_surface_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, next_buffer_done()).Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::create_surface_done));
            client.wait_for_create_surface();

            client.display_server.next_buffer(
                0,
                &client.surface.id(),
                client.surface.mutable_buffer(),
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::next_buffer_done));

            client.wait_for_next_buffer();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(ApplicationMediatorReport, session_release_surface_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport>
        the_application_mediator_report()
        {
            auto result = std::make_shared<MockApplicationMediatorReport>();

            EXPECT_CALL(*result, session_release_surface_called(testing::_)).
                Times(1);

            return result;
        }
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            mt::TestProtobufClient client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, create_surface_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, next_buffer_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, release_surface_done()).Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::create_surface_done));
            client.wait_for_create_surface();

            client.display_server.next_buffer(
                0,
                &client.surface.id(),
                client.surface.mutable_buffer(),
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::next_buffer_done));

            client.wait_for_next_buffer();

            client.display_server.release_surface(
                0,
                &client.surface.id(),
                &client.ignored,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::release_surface_done));

            client.wait_for_release_surface();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(ApplicationMediatorReport, session_disconnect_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport>
        the_application_mediator_report()
        {
            auto result = std::make_shared<MockApplicationMediatorReport>();

            EXPECT_CALL(*result, session_disconnect_called(testing::_)).
                Times(1);

            return result;
        }
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            mt::TestProtobufClient client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, create_surface_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, next_buffer_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, release_surface_done()).Times(testing::AtLeast(0));
            EXPECT_CALL(client, disconnect_done()).Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::create_surface_done));
            client.wait_for_create_surface();

            client.display_server.next_buffer(
                0,
                &client.surface.id(),
                client.surface.mutable_buffer(),
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::next_buffer_done));

            client.wait_for_next_buffer();

            client.display_server.release_surface(
                0,
                &client.surface.id(),
                &client.ignored,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::release_surface_done));

            client.wait_for_release_surface();

            client.display_server.disconnect(
                0,
                &client.ignored,
                &client.ignored,
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::disconnect_done));

            client.wait_for_disconnect_done();
        }
    } client_process;

    launch_client_process(client_process);
}
