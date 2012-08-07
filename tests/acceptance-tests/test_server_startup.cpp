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
 *              Thomas Voss <thomas.voss@canonical.com>
 */

#include "display_server_test_fixture.h"
#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/thread/all.h"

#include "mir_protobuf.pb.h"
#include "mir_client/mir_surface.h"

#include <chrono>
#include <cstdio>
#include <functional>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;

namespace
{
class StubDisplayServer : public mir::protobuf::DisplayServer
{
public:
    void connect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::ConnectMessage* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        // TODO do the real work
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());

        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        done->Run();
    }
};

class StubCommunicator : public mf::Communicator
{
public:
    StubCommunicator(const std::string& socket_file)
    : communicator(socket_file, &display_server)
    {
    }

    void start()
    {
        communicator.start();
    }

    StubDisplayServer display_server;
    mir::frontend::ProtobufAsioCommunicator communicator;
};
}

TEST_F(BespokeDisplayServerTestFixture, server_announces_itself_on_startup)
{
    ASSERT_FALSE(mir::detect_server(mir::test_socket_file(), std::chrono::milliseconds(0)));

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            return std::make_shared<StubCommunicator>(mir::test_socket_file());
        }

        void exec(mir::DisplayServer *)
        {
        }
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            EXPECT_TRUE(mir::detect_server(mir::test_socket_file(),
                                           std::chrono::milliseconds(100)));
        }
    } client_config;

    launch_client_process(client_config);
}

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

}

TEST_F(BespokeDisplayServerTestFixture,
       a_connection_attempt_results_in_a_session)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            return std::make_shared<mf::ProtobufAsioCommunicator>(
                mir::test_socket_file(),
                &counter);
        }

        void on_exit(mir::DisplayServer* )
        {
            std::unique_lock<std::mutex> lock(counter.guard);
            if (counter.session_count != 1)
                counter.wait_condition.wait_for(lock, std::chrono::milliseconds(100));
            EXPECT_EQ(1, counter.session_count);
        }
        SessionCounter counter;
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            using ::mir::client::Surface;

            Surface mysurface(mir::test_socket_file(), 640, 480, 0);
            EXPECT_TRUE(mysurface.is_valid());
        }
    } client_config;

    launch_client_process(client_config);
}

#ifdef MIR_TODO // Sessions are no longer visible
namespace
{
struct SessionCollector
{
    SessionCollector() : sessions()
    {
    }

    SessionCollector(SessionCounter const &) = delete;

    void on_session_state(std::shared_ptr<mf::Session> const& session, mf::SessionState)
    {
        std::unique_lock<std::mutex> lock(guard);
        sessions[session->id()] = session;
        wait_condition.notify_one();
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    std::map<int, std::shared_ptr<mf::Session>> sessions;
};
}

TEST_F(BespokeDisplayServerTestFixture,
       each_connection_attempt_results_in_a_session_with_a_unique_id)
{
    std::size_t const connections = 5;

    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(int const connections) : connections(connections) {}

        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            auto comm(std::make_shared<mf::ProtobufAsioCommunicator>(mir::test_socket_file()));
            comm->signal_session_state().connect(
                boost::bind(&SessionCollector::on_session_state, &collector, _1, _2));
            return comm;
        }

        void on_exit(mir::DisplayServer* )
        {
            std::unique_lock<std::mutex> lock(collector.guard);
            while (collector.sessions.size() != connections)
                collector.wait_condition.wait_for(lock, std::chrono::milliseconds(1));
            EXPECT_EQ(connections, collector.sessions.size());

            collector.sessions.clear();
        }

        SessionCollector collector;
        std::size_t connections;
    } server_config(connections);

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            namespace ba = boost::asio;
            namespace bal = boost::asio::local;
            namespace bs = boost::system;

            ba::io_service io_service;
            bal::stream_protocol::endpoint endpoint(mir::test_socket_file());
            bal::stream_protocol::socket socket(io_service);

            bs::error_code error;
            socket.connect(endpoint, error);

            EXPECT_TRUE(!error);
        }
    } client_config;

    for (std::size_t i = 0; i != connections; ++i)
        launch_client_process(client_config);
}
#endif
