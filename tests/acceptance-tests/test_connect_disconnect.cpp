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
 *              Thomas Guest <thomas.guest@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "display_server_test_fixture.h"

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mp = mir::process;

TEST_F(DefaultDisplayServerTestFixture, client_connects_and_disconnects)
{
    namespace ba = boost::asio;
    namespace bal = boost::asio::local;
    namespace bs = boost::system;

    struct Client : TestingClientConfiguration
    {
        ba::io_service io_service;
        bal::stream_protocol::endpoint endpoint;
        bal::stream_protocol::socket socket;

        Client() : endpoint(mir::test_socket_file()), socket(io_service) {}

        bs::error_code connect()
        {
            bs::error_code error;
            socket.connect(endpoint, error);
            return error;
        }

        bs::error_code disconnect()
        {
            bs::error_code error;
            ba::write(socket, ba::buffer(std::string("disconnect\n")), error);
            EXPECT_FALSE(error);
            return error;
        }

        void exec()
        {
            EXPECT_EQ(bs::errc::success, connect());
            EXPECT_EQ(bs::errc::success, disconnect());
        }
    } client_connects_and_disconnects;

    launch_client_process(client_connects_and_disconnects);
}
