/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_BASIC_CLIENT_SERVER_FIXTURE_H_
#define MIR_TEST_FRAMEWORK_BASIC_CLIENT_SERVER_FIXTURE_H_

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"

#include "boost/throw_exception.hpp"

namespace mir_test_framework
{
/// Test fixture to create a server in process and allocate a client connection.
/// There are a lot of tests that could use this context.
template<class TestServerConfiguration>
struct BasicClientServerFixture : InProcessServer
{
    TestServerConfiguration server_configuration;
    UsingStubClientPlatform using_stub_client_platform;

    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }

    void SetUp() override
    {
        InProcessServer::SetUp();
        connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
        if (!mir_connection_is_valid(connection))
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error{std::string{"Failed to connect to test server: "} +
                                   mir_connection_get_error_message(connection)});
        }
    }

    void TearDown() override
    {
        mir_connection_release(connection);
        InProcessServer::TearDown();
    }

    MirConnection* connection = nullptr;
};
}

#endif /* MIR_TEST_FRAMEWORK_BASIC_CLIENT_SERVER_FIXTURE_H_ */
