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

#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <list>

namespace mp = mir::process;
namespace mf = mir::frontend;

// We need some tests to prove that errors are reported by the
// display server test fixture.  But don't want them to fail in
// normal builds.

namespace
{
struct StubCommunicator : public mf::Communicator
{
    StubCommunicator()
    {
    }

    void run()
    {
    }
};

}

//#define MIR_INCLUDE_TESTS_MEANT_TO_FAIL
#ifdef MIR_INCLUDE_TESTS_MEANT_TO_FAIL
TEST_F(BespokeDisplayServerTestFixture, failing_server_side_test)
{
    struct Server : TestingServerConfiguration
    {
        void exec(mir::DisplayServer* )
        {
            using namespace testing;
            FAIL() << "Proving a test can fail";
        }
    } fail;

    launch_server_process(fail);
}

TEST_F(BespokeDisplayServerTestFixture, failing_without_server)
{
}
#endif

TEST_F(BespokeDisplayServerTestFixture, demonstrate_multiple_clients)
{
    struct Server : TestingServerConfiguration
    {
        void exec(mir::DisplayServer* )
        {
            // empty function
        }
    } empty_function;

    launch_server_process(empty_function);

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

TEST_F(BespokeDisplayServerTestFixture, client_connects_and_disconnects)
{
    struct Server : TestingServerConfiguration
    {
        void exec(mir::DisplayServer* )
        {
            // empty function
        }
    } empty_function;

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            std::shared_ptr<mf::Communicator> communicator(new StubCommunicator());
            mf::Application application(communicator);

            EXPECT_NO_THROW(application.connect());
            EXPECT_NO_THROW(application.disconnect());
        }
    } client_connects_and_disconnects;

    launch_server_process(empty_function);

    launch_client_process(client_connects_and_disconnects);
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
