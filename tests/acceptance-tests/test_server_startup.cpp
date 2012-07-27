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


TEST_F(BespokeDisplayServerTestFixture, server_announces_itself_on_startup)
{
    ASSERT_FALSE(mir::detect_server(mir::test_socket_file(), std::chrono::milliseconds(0)));

    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(std::string const& file) : socket_file(file)
        {
        }

        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            return std::make_shared<mir::frontend::ProtobufAsioCommunicator>(socket_file);
        }

        std::string const socket_file;
    } server_config(mir::test_socket_file());

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(std::string const& socket_file) : socket_file(socket_file)
        {
        }

        void exec()
        {
            EXPECT_TRUE(mir::detect_server(socket_file, std::chrono::milliseconds(100)));
        }

        std::string const socket_file;
    } client_config(mir::test_socket_file());

    launch_client_process(client_config);
}

struct SessionSignalCollector
{
    SessionSignalCollector() : session_count(0)
    {
    }

    SessionSignalCollector(SessionSignalCollector const &) = delete;

    void on_new_session()
    {
        session_count++;
    }

    int session_count;
};

TEST_F(BespokeDisplayServerTestFixture,
       a_connection_attempt_results_in_a_session)
{
    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(std::string const& socket_file)
            : socket_file(socket_file)
        {
        }

        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            auto comm(std::make_shared<mir::frontend::ProtobufAsioCommunicator>(socket_file));
            comm->signal_new_session().connect(
                std::bind(
                    &SessionSignalCollector::on_new_session,
                    &collector));
            return comm;
        }

        void on_exit(mir::DisplayServer* )
        {
            EXPECT_EQ(int {1}, collector.session_count);
        }

        std::string const socket_file;
        SessionSignalCollector collector;
    } server_config(mir::test_socket_file());

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(std::string const& socket_file) : socket_file(socket_file)
        {
        }

        void exec()
        {
            EXPECT_TRUE(mir::detect_server(socket_file, std::chrono::milliseconds(100)));
        }
        std::string const socket_file;
    } client_config(mir::test_socket_file());

    launch_client_process(client_config);
}

