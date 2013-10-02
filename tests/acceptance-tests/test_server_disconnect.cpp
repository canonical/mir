/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;

using ServerDisconnect = mtf::BespokeDisplayServerTestFixture;

namespace
{
struct MockEventHandler
{
    MOCK_METHOD1(handle, void(MirLifecycleState transition));
};

void my_event_handler(MirConnection*, MirLifecycleState transition, void*);

struct MyTestingClientConfiguration : mtf::TestingClientConfiguration
{
    void exec() override
    {
        MockEventHandler mock_event_handler;

        auto connection = mir_connect_sync(mtf::test_socket_file().c_str() , __PRETTY_FUNCTION__);
        mir_connection_set_lifecycle_event_callback(connection, &my_event_handler, &mock_event_handler);

        EXPECT_CALL(mock_event_handler, handle(mir_lifecycle_connection_lost)).Times(1);
    }
};


void my_event_handler(MirConnection*, MirLifecycleState transition, void* event_handler)
{
    static_cast<MockEventHandler*>(event_handler)->handle(transition);

    if (transition == mir_lifecycle_connection_lost)
    {
        raise(SIGTERM);
    }
}
}

TEST_F(ServerDisconnect, client_detects_server_shutdown)
{
    TestingServerConfiguration server_config;
    launch_server_process(server_config);

}



