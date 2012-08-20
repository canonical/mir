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
#include <gmock/gmock.h>

#include <memory>
#include <vector>

namespace mf = mir::frontend;

namespace mir
{
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

struct NullDeleter
{
    void operator()(void* )
    {
    }
};

class MockLogger : public mir::client::Logger
{
    virtual std::ostream& error()
    {
        return std::cerr;
    }
};

class MockIpcFactory : public mf::ProtobufIpcFactory
{
public:
    MockIpcFactory(mir::protobuf::DisplayServer& server) :
        server(&server, NullDeleter())
    {
        using namespace testing;

        ON_CALL(*this, make_ipc_server()).WillByDefault(Return(this->server));

        // called during initialisation:
        // there's always a server awaiting the next connection
        EXPECT_CALL(*this, make_ipc_server()).Times(1);
    }

    MOCK_METHOD0(make_ipc_server, std::shared_ptr<mir::protobuf::DisplayServer>());

private:
    std::shared_ptr<mir::protobuf::DisplayServer> server;
};

struct ProtobufAsioCommunicatorTestFixture : public ::testing::Test
{
    static const std::string& socket_name()
    {
        static std::string socket_name("./mir_test_pb_asio_socket");
        return socket_name;
    }

    ProtobufAsioCommunicatorTestFixture() :
        factory(std::make_shared<MockIpcFactory>(collector)),
        comm(socket_name(), factory),
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
    std::shared_ptr<MockIpcFactory> factory;
    mf::ProtobufAsioCommunicator comm;

    // "Client" side
    mir::client::MirRpcChannel channel;
    mir::protobuf::DisplayServer::Stub display_server;
    mir::protobuf::ConnectMessage connect_message;
    mir::protobuf::Surface surface;
    mir::protobuf::Void ignored;
};


struct ProtobufAsioMultiClientCommunicatorTestFixture : public ::testing::Test
{
    static int const connection_count = 10;

    static const std::string& socket_name()
    {
        static std::string socket_name("./mir_test_pb_asio_socket");
        return socket_name;
    }

    ProtobufAsioMultiClientCommunicatorTestFixture() :
        factory(std::make_shared<MockIpcFactory>(collector)),
        comm(socket_name(), factory)
    {
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
    std::shared_ptr<MockIpcFactory> factory;
    mf::ProtobufAsioCommunicator comm;


    struct Client
    {
        Client() :
            channel(ProtobufAsioMultiClientCommunicatorTestFixture::socket_name(), std::make_shared<MockLogger>()),
            display_server(&channel)
        {
            connect_message.set_width(640);
            connect_message.set_height(480);
            connect_message.set_pixel_format(0);
        }

        mir::client::MirRpcChannel channel;
        mir::protobuf::DisplayServer::Stub display_server;
        mir::protobuf::ConnectMessage connect_message;
        mir::protobuf::Surface surface;
        mir::protobuf::Void ignored;
    };

    Client client[connection_count];
};
}

TEST_F(ProtobufAsioCommunicatorTestFixture, connection_results_in_a_callback)
{
    EXPECT_CALL(*factory, make_ipc_server()).Times(1);

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
    EXPECT_CALL(*factory, make_ipc_server()).Times(1);

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
    EXPECT_CALL(*factory, make_ipc_server()).Times(1);

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
    EXPECT_CALL(*factory, make_ipc_server()).Times(1);

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
    EXPECT_CALL(*factory, make_ipc_server()).Times(1);

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

TEST_F(ProtobufAsioMultiClientCommunicatorTestFixture,
       multiple_clients_can_connect_and_disconnect)
{
    EXPECT_CALL(*factory, make_ipc_server()).Times(connection_count);

    for (int i = 0; i != connection_count; ++i)
    {
        client[i].display_server.connect(
            0,
            &client[i].connect_message,
            &client[i].surface,
            google::protobuf::NewCallback(&mir::client::done));
    }

    expect_session_count(connection_count);
    expect_connected_session_count(connection_count);

    for (int i = 0; i != connection_count; ++i)
    {
        client[i].display_server.disconnect(
            0,
            &client[i].ignored,
            &client[i].ignored,
            google::protobuf::NewCallback(&mir::client::done));
    }

    expect_session_count(connection_count);
    expect_connected_session_count(0);
}
}
