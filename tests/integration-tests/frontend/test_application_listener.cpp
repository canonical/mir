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
 */

#include "mir_client/mir_client_library.h"
#include "mir/frontend/application_listener.h"

#include "mir_test/display_server_test_fixture.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;

TEST_F(BespokeDisplayServerTestFixture, application_listener_is_notified)
{
    struct Server : TestingServerConfiguration
    {
//        void exec(mir::DisplayServer* display_server)
//        {
//        }
    } server_processing;

    launch_server_process(server_processing);

//    struct ClientConfig : ClientConfigCommon
//    {
//        void exec()
//        {
//            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));
//
//            ASSERT_TRUE(connection != NULL);
//            EXPECT_TRUE(mir_connection_is_valid(connection));
//            EXPECT_STREQ(mir_connection_get_error_message(connection), "");
//
//            mir_connection_release(connection);
//        }
//    } client_config;
//
//    launch_client_process(client_config);
}
