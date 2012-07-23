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

#include "display_server_test_fixture.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <list>

namespace mp = mir::process;

// We need some tests to prove that errors are reported by the
// display server test fixture.  But don't want them to fail in
// normal builds.

namespace
{
void empty_function()
{
}

void demo()
{
    SCOPED_TRACE("Client");
}
}

//#define MIR_INCLUDE_TESTS_MEANT_TO_FAIL
#ifdef MIR_INCLUDE_TESTS_MEANT_TO_FAIL
TEST_F(DisplayServerTestFixture, failing_server_side_test)
{
    launch_server_process([&]() -> void
    {
        using namespace testing;
        FAIL() << "Proving a test can fail";
    });
}

TEST_F(DisplayServerTestFixture, failing_without_server)
{
}
#endif

TEST_F(DisplayServerTestFixture, demonstrate_multiple_clients)
{
    launch_server_process(empty_function);

    for(int i = 0; i != 10; ++i)
    {
        launch_client_process(demo);
    }
}
