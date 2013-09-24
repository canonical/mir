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

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir/frontend/connector.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <list>

namespace mf = mir::frontend;

// We need some tests to prove that errors are reported by the
// display server test fixture.  But don't want them to fail in
// normal builds.
TEST_F(BespokeDisplayServerTestFixture, DISABLED_failing_server_side_test)
{
    struct Server : TestingServerConfiguration
    {
        void exec()
        {
            using namespace testing;
            FAIL() << "Proving a test can fail";
        }
    } fail;

    launch_server_process(fail);
}

TEST_F(BespokeDisplayServerTestFixture, DISABLED_failing_without_server)
{
}

TEST_F(DefaultDisplayServerTestFixture, demonstrate_multiple_clients)
{
    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            SCOPED_TRACE("Demo Client");
        }
    } demo;

    for(int i = 0; i != 10; ++i)
    {
        launch_client_process(demo);
    }
}
