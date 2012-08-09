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
 */

#include "mir/frontend/protobuf_asio_communicator.h"

#include "mir_protobuf.pb.h"
#include "mir_client/mir_rpc_channel.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace mf = mir::frontend;

namespace
{
struct SessionCounter : mir::protobuf::DisplayServer
{
    int session_count;
    int connected_sessions;
    std::mutex guard;
    std::condition_variable wait_condition;

    SessionCounter() : session_count(0), connected_sessions(0)
    {
    }

    SessionCounter(SessionCounter const &) = delete;
    void connect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::ConnectMessage* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());

        std::unique_lock<std::mutex> lock(guard);
        ++session_count;
        ++connected_sessions;
        wait_condition.notify_one();

        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        std::unique_lock<std::mutex> lock(guard);
        --connected_sessions;
        wait_condition.notify_one();
        done->Run();
    }
};

class MockLogger : public mir::client::Logger
{
    virtual std::ostream& error()
    {
        return std::cerr;
    }
};

struct ProtobufAsioCommunicatorTestFixture : public ::testing::Test
{
    static const std::string& socket_name()
    {
        static std::string socket_name("/tmp/mir_test_pb_asio_socket");
        return socket_name;
    }

    ProtobufAsioCommunicatorTestFixture() :
        comm(socket_name(), &collector),
        channel(socket_name(), std::make_shared<MockLogger>()),
        display_server(&channel)
    {
        connect_message.set_width(640);
        connect_message.set_height(480);
        connect_message.set_pixel_format(0);
    }

    void SetUp()
    {
        comm.start();
    }

    void expect_session_count(int expected_count)
    {
        std::unique_lock<std::mutex> ul(collector.guard);
        for (int ntries = 20;
             ntries-- != 0 && collector.session_count != expected_count; )
        {
            collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));
        }
        EXPECT_EQ(expected_count, collector.session_count);
    }

    void expect_connected_session_count(int expected_count)
    {
        std::unique_lock<std::mutex> ul(collector.guard);
        for (int ntries = 20;
             ntries-- != 0 && collector.connected_sessions != expected_count; )
        {
            collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));
        }
        EXPECT_EQ(expected_count, collector.connected_sessions);
    }

    // "Server" side
    SessionCounter collector;
    mf::ProtobufAsioCommunicator comm;

    // "Client" side
    mir::client::MirRpcChannel channel;
    mir::protobuf::DisplayServer::Stub display_server;
    mir::protobuf::ConnectMessage connect_message;
    mir::protobuf::Surface surface;
    mir::protobuf::Void ignored;
};
}

TEST_F(ProtobufAsioCommunicatorTestFixture, connection_results_in_a_callback)
{
    display_server.connect(
        0,
        &connect_message,
        &surface,
        google::protobuf::NewCallback(&mir::client::done));

    expect_session_count(1);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
        a_connection_attempt_results_in_a_session_being_connected)
{
    display_server.connect(
        0,
        &connect_message,
        &surface,
        google::protobuf::NewCallback(&mir::client::done));

    expect_connected_session_count(1);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       each_connection_attempt_results_in_a_new_session_being_created)
{
    int const connection_count{5};

    for (int i = 0; i != connection_count; ++i)
    {
        display_server.connect(
            0,
            &connect_message,
            &surface,
            google::protobuf::NewCallback(&mir::client::done));
    }

    expect_session_count(connection_count);
    expect_connected_session_count(connection_count);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       connect_then_disconnect_a_session)
{
    display_server.connect(
        0,
        &connect_message,
        &surface,
        google::protobuf::NewCallback(&mir::client::done));

    expect_connected_session_count(1);

    display_server.disconnect(
        0,
        &ignored,
        &ignored,
        google::protobuf::NewCallback(&mir::client::done));

    expect_connected_session_count(0);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       double_disconnection_attempt_has_no_effect)
{
    display_server.connect(
        0,
        &connect_message,
        &surface,
        google::protobuf::NewCallback(&mir::client::done));

    expect_connected_session_count(1);

    display_server.disconnect(
        0,
        &ignored,
        &ignored,
        google::protobuf::NewCallback(&mir::client::done));

    expect_connected_session_count(0);

    display_server.disconnect(
        0,
        &ignored,
        &ignored,
        google::protobuf::NewCallback(&mir::client::done));

    expect_connected_session_count(0);
}
