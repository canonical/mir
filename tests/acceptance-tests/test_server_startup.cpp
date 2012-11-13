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

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir/chrono/chrono.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;

namespace mir
{
TEST_F(BespokeDisplayServerTestFixture, server_announces_itself_on_startup)
{
    ASSERT_FALSE(mir::detect_server(mir::test_socket_file(), std::chrono::milliseconds(0)));

    TestingServerConfiguration server_config;

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
}
