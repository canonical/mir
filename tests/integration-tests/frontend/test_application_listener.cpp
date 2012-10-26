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

#include "mir_client/mir_client_library.h"
#include "mir/frontend/application_listener.h"

#include "mir_test/display_server_test_fixture.h"
#include "mir_test/test_client.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mt = mir::test;

namespace
{
// If we didn't support g++4.4 these could be nested in
//  application_listener_XXX_is_notified::Server::make_applicaton_listener
// where it belongs (and could have shorter names)
struct MockApplicationListenerForConnect : mf::NullApplicationListener
{
    MOCK_METHOD1(application_connect_called, void (std::string const&));
};

struct MockApplicationListenerForCreateSurface : mf::NullApplicationListener
{
    MOCK_METHOD1(application_create_surface_called, void (std::string const&));
};
}

TEST_F(BespokeDisplayServerTestFixture, application_listener_connect_is_notified)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::ApplicationListener>
        make_application_listener()
        {
            auto result = std::make_shared<MockApplicationListenerForConnect>();

            EXPECT_CALL(*result, application_connect_called(testing::_)).
                Times(1);

            return result;
        }
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            mt::TestClient client(mir::test_socket_file());

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).
                Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(&client, &mt::TestClient::connect_done));

            client.wait_for_connect_done();
        }
    } client_process;

    launch_client_process(client_process);
}

TEST_F(BespokeDisplayServerTestFixture, application_listener_create_surface_is_notified)
{
    struct Server : TestingServerConfiguration
    {
        std::shared_ptr<mf::ApplicationListener>
        make_application_listener()
        {
            auto result = std::make_shared<MockApplicationListenerForCreateSurface>();

            EXPECT_CALL(*result, application_create_surface_called(testing::_)).
                Times(1);

            return result;
        }
    } server_processing;

    launch_server_process(server_processing);

    struct Client: TestingClientConfiguration
    {
        void exec()
        {
            mt::TestClient client(mir::test_socket_file());

            client.connect_parameters.set_application_name(__PRETTY_FUNCTION__);
            EXPECT_CALL(client, connect_done()).
                Times(testing::AtLeast(0));
            EXPECT_CALL(client, create_surface_done()).
                Times(testing::AtLeast(0));

            client.display_server.connect(
                0,
                &client.connect_parameters,
                &client.connection,
                google::protobuf::NewCallback(&client, &mt::TestClient::connect_done));

            client.wait_for_connect_done();

            client.display_server.create_surface(
                0,
                &client.surface_parameters,
                &client.surface,
                google::protobuf::NewCallback(&client, &mt::TestClient::create_surface_done));
            client.wait_for_create_surface();

        }
    } client_process;

    launch_client_process(client_process);
}
