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
struct MockSessionMediatorReport : mf::SessionMediatorReport
{
    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_next_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_exchange_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));
    MOCK_METHOD2(session_start_prompt_session_called, void (std::string const&, pid_t));
    MOCK_METHOD1(session_stop_prompt_session_called, void (std::string const&));

    void session_drm_auth_magic_called(const std::string&) override {};
    void session_configure_surface_called(std::string const&) override {};
    void session_configure_surface_cursor_called(std::string const&) override {};
    void session_configure_display_called(std::string const&) override {};
    void session_error(const std::string&, const char*, const std::string&) override {};
};

const int rpc_timeout_ms{100000};

typedef BespokeDisplayServerTestFixture SessionMediatorReport;
}

TEST_F(SessionMediatorReport, session_connect_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_connect_called(testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            testing::NiceMock<mt::TestProtobufClient> client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).
                Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(SessionMediatorReport, session_create_surface_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_create_surface_called(testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
    } server_processing;
    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            testing::NiceMock<mt::TestProtobufClient> client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).
                Times(testing::AtLeast(0));
            EXPECT_CALL(client, create_surface_done()).
                Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::create_surface_done));
            client.wait_for_create_surface();

        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(SessionMediatorReport, session_next_buffer_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_next_buffer_called(testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
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

TEST_F(SessionMediatorReport, session_exchange_buffer_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_exchange_buffer_called(testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
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
            EXPECT_CALL(client, exchange_buffer_done()).Times(testing::AtLeast(0));

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

            mir::protobuf::BufferRequest request;
            *request.mutable_id() =  client.surface.id();
            *request.mutable_buffer() =  client.surface.buffer();

            client.display_server.exchange_buffer(
                0,
                &request,
                client.surface.mutable_buffer(),
                google::protobuf::NewCallback(&client, &mt::TestProtobufClient::exchange_buffer_done));

            client.wait_for_exchange_buffer();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(SessionMediatorReport, session_release_surface_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_release_surface_called(testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
    } server_processing;
    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            testing::NiceMock<mt::TestProtobufClient> client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::create_surface_done));
            client.wait_for_create_surface();

            client.display_server.next_buffer(
                0,
                &client.surface.id(),
                client.surface.mutable_buffer(),
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::next_buffer_done));

            client.wait_for_next_buffer();

            client.display_server.release_surface(
                0,
                &client.surface.id(),
                &client.ignored,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::release_surface_done));

            client.wait_for_release_surface();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(SessionMediatorReport, session_disconnect_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_disconnect_called(testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
    } server_processing;
    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            testing::NiceMock<mt::TestProtobufClient> client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::create_surface_done));
            client.wait_for_create_surface();

            client.display_server.next_buffer(
                0,
                &client.surface.id(),
                client.surface.mutable_buffer(),
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::next_buffer_done));

            client.wait_for_next_buffer();

            client.display_server.release_surface(
                0,
                &client.surface.id(),
                &client.ignored,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::release_surface_done));

            client.wait_for_release_surface();

            client.display_server.disconnect(
                0,
                &client.ignored,
                &client.ignored,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::disconnect_done));

            client.wait_for_disconnect_done();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(SessionMediatorReport, prompt_session_start_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_start_prompt_session_called(testing::_, testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
    } server_processing;
    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            testing::NiceMock<mt::TestProtobufClient> client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.start_prompt_session(
                0,
                &client.prompt_session_parameters,
                &client.prompt_session,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::prompt_session_start_done));

            client.wait_for_prompt_session_start_done();

        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(SessionMediatorReport, prompt_session_stop_called)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionMediatorReport> the_session_mediator_report() override
        {
            if (!report)
            {
                report = std::make_shared<testing::NiceMock<MockSessionMediatorReport>>();
                EXPECT_CALL(*report, session_stop_prompt_session_called(testing::_)).
                    Times(1);
            }
            return report;
        }
        std::shared_ptr<MockSessionMediatorReport> report;
    } server_processing;
    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            testing::NiceMock<mt::TestProtobufClient> client(mtf::test_socket_file(), rpc_timeout_ms);

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.start_prompt_session(
                0,
                &client.prompt_session_parameters,
                &client.prompt_session,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::prompt_session_start_done));

            client.wait_for_prompt_session_start_done();

            client.display_server.stop_prompt_session(
                0,
                &client.ignored,
                &client.ignored,
                google::protobuf::NewCallback(
                    static_cast<mt::TestProtobufClient*>(&client),
                    &mt::TestProtobufClient::prompt_session_stop_done));

            client.wait_for_prompt_session_stop_done();
        }
    } client_process;

    launch_client_process(client_process);
}
