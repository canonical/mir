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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_protobuf.pb.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir/client/private.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
using DemoServerConfiguration = mir_test_framework::StubbedServerConfiguration;

struct DemoSocketFDServer : mir_test_framework::InProcessServer
{
    DemoServerConfiguration my_server_config;

    mir::DefaultServerConfiguration& server_config() override { return my_server_config; }
};
}

TEST_F(DemoSocketFDServer, client_gets_fd)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto const rpc_channel = mir::client::the_rpc_channel(connection);

    using namespace mir::protobuf;
    using namespace google::protobuf;

    DisplayServer::Stub server(rpc_channel.get());

    SocketFDRequest parameters;
    parameters.set_number(1);

    SocketFD result;

    server.client_socket_fd(
        nullptr,
        &parameters,
        &result,
        NewCallback([]{}));

    mir_connection_release(connection);

    EXPECT_THAT(result.fd_size(), Eq(1));
}
