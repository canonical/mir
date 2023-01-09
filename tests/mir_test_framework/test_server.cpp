/*
 * Copyright © Canonical Ltd.
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
 */

#include <miral/test_server.h>

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
    stop_server();
    testing::Test::TearDown();
}

using miral::TestServer;
