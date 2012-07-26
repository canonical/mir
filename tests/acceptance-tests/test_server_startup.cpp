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
#include "mir/frontend/communicator.h"

#include <boost/asio.hpp>

#include <chrono>
#include <cstdio>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


namespace mir
{

bool detect_server(
        const std::string& socket_file,
        std::chrono::milliseconds const& /*timeout*/ )
{
    namespace ba = boost::asio;
    namespace bal = boost::asio::local;
    ba::io_service io_service;
    bal::stream_protocol::endpoint endpoint(socket_file);
    bal::stream_protocol::socket socket(io_service);

    try
    {
        socket.connect(endpoint);
    } catch(const boost::system::system_error&)
    {
        return false;
    }

    return true;
}

class ProtobufAsioCommunicator : public mir::frontend::Communicator
{
public:
    ProtobufAsioCommunicator(std::string const& socket_file) : socket_file(socket_file)
    {
        namespace ba = boost::asio;
        namespace bal = boost::asio::local;
        ba::io_service io_service;
        bal::stream_protocol::acceptor acceptor(io_service, socket_file);
        bal::stream_protocol::socket socket(io_service);

        acceptor.accept(socket);
    }

private:
    std::string const socket_file;
};
}

TEST_F(BespokeDisplayServerTestFixture, server_announces_itself_on_startup)
{
    const std::string socket_file{"/tmp/mir_socket_test"};
    ASSERT_FALSE(mir::detect_server(socket_file, std::chrono::milliseconds(100)));

    struct Server : TestingServerConfiguration
    {
        std::string const socket_file;
        Server(std::string const& file) : socket_file(file)
        {
            std::remove(file.c_str());
        }

        std::shared_ptr<mir::frontend::Communicator> make_communicator()
        {
            return std::make_shared<mir::ProtobufAsioCommunicator>(socket_file);
        }

        void exec(mir::DisplayServer *)
        {
            make_communicator();
        }

    } default_config(socket_file);

    launch_server_process(default_config);

    struct Client : TestingClientConfiguration
    {
        std::string const socket_file;
        Client(std::string const& socket_file) : socket_file(socket_file)
        {
        }

        void exec()
        {
            std::cout << "DEBUG testing server exists" << std::endl;
            EXPECT_TRUE(mir::detect_server(socket_file, std::chrono::milliseconds(100)));
        }
    } client(socket_file);

    launch_client_process(client);

}

