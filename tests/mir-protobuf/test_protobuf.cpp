/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "test_protobuf.pb.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir/client/private.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"

#include <gtest/gtest.h>

#include <atomic>

namespace
{
class DemoPrivateProtobuf : public mir_test_framework::InProcessServer
{
    mir::DefaultServerConfiguration& server_config() override { return server_config_; }

    mir_test_framework::StubbedServerConfiguration server_config_;
};

void callback(std::atomic<bool>* called_back) { called_back->store(true); }
char const* const nothing_returned = "Nothing returned";
}

TEST_F(DemoPrivateProtobuf, client_can_call_server)
{
    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto const rpc_channel = mir::client::the_rpc_channel(connection);

    using namespace mir::protobuf;
    using namespace google::protobuf;

    MirServer::Stub server(rpc_channel.get());

    Parameters parameters;
    parameters.set_name(__PRETTY_FUNCTION__);

    Result result;
    result.set_error(nothing_returned);
    std::atomic<bool> called_back{false};

    // Note:
    // As the default server won't recognise this call it drops the connection
    // resulting in a callback when the connection drops (but result being unchanged)
    server.function(
        nullptr,
        &parameters,
        &result,
        NewCallback(&callback, &called_back));

    mir_connection_release(connection);

    EXPECT_TRUE(called_back);
    EXPECT_EQ(nothing_returned, result.error());
}
