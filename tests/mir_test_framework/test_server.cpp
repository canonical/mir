/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 */

#include <miral/test_server.h>

#include <mir/client/connection.h>

using namespace testing;
using namespace std::chrono_literals;

void miral::TestServer::SetUp()
{
    if (start_server_in_setup)
        start_server();
    testing::Test::SetUp();
}

void miral::TestServer::TearDown()
{
    // There's a race between closing a client and closing the server.
    // AutoSendBuffer is trying to send *after* SessionMediator is destroyed.
    // This sleep() is not a good fix, but a good fix would be deep in legacy code.
    std::this_thread::sleep_for(100ms);

    stop_server();
    testing::Test::TearDown();
}

using miral::TestServer;

// Minimal test to ensure the server runs and exits
TEST_F(TestServer, connect_client_works)
{
    auto const connection = connect_client(__PRETTY_FUNCTION__);

    EXPECT_TRUE(mir_connection_is_valid(connection));
}
