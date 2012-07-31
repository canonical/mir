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

#include <chrono>
#include <cstdio>
#include <functional>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;

TEST_F(BespokeDisplayServerTestFixture, server_announces_itself_on_startup)
{
    ASSERT_FALSE(mir::detect_server(mir::test_socket_file(), std::chrono::milliseconds(0)));

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            return std::make_shared<mf::ProtobufAsioCommunicator>(mir::test_socket_file());
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
struct SessionCounter
{
    SessionCounter() : session_count(0)
    {
    }

    SessionCounter(SessionCounter const &) = delete;

    void on_session_event()
    {
        std::unique_lock<std::mutex> lock(guard);
        session_count++;
        wait_condition.notify_one();
    }

    int session_count;
    std::mutex guard;
    std::condition_variable wait_condition;
};
}

TEST_F(BespokeDisplayServerTestFixture,
       a_connection_attempt_results_in_a_session)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            auto comm(std::make_shared<mf::ProtobufAsioCommunicator>(mir::test_socket_file()));
            comm->signal_session_event().connect(
                std::bind(&SessionCounter::on_session_event, &collector));
            return comm;
        }

        void on_exit(mir::DisplayServer* )
        {
            std::unique_lock<std::mutex> lock(collector.guard);
            while (collector.session_count != 1)
                collector.wait_condition.wait_for(lock, std::chrono::milliseconds(1));
            EXPECT_EQ(1, collector.session_count);
        }
        SessionCounter collector;
    } server_config;

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

    launch_client_process(client_config);
}

namespace
{
struct SessionCollector
{
    SessionCollector() : sessions()
    {
    }

    SessionCollector(SessionCounter const &) = delete;

    void on_session_event(std::shared_ptr<mf::Session> const& session, mf::SessionEvent)
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
            comm->signal_session_event().connect(
                boost::bind(&SessionCollector::on_session_event, &collector, _1, _2));
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
