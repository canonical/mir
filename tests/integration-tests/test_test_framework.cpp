/*
 * Copyright © 2012, 2013 Canonical Ltd.
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
#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test/signal.h"

#include "mir_toolkit/mir_client_library.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <list>
#include <sys/resource.h>

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

namespace
{
struct DemoInProcessServer : mir_test_framework::InProcessServer
{
    virtual mir::DefaultServerConfiguration& server_config() { return server_config_; }

    mir_test_framework::StubbedServerConfiguration server_config_;
};

struct DemoInProcessServerWithStubClientPlatform : DemoInProcessServer
{
    mir_test_framework::UsingStubClientPlatform using_stub_client_platform;
};
}

TEST_F(DemoInProcessServer, client_can_connect)
{
    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    EXPECT_TRUE(mir_connection_is_valid(connection));

    mir_connection_release(connection);
}

// Regression test for https://bugs.launchpad.net/mir/+bug/1395762
TEST_F(DemoInProcessServerWithStubClientPlatform, surface_creation_does_not_leak_fds)
{
    mir::test::Signal connection_released;

    std::thread{
        [&]
        {
            auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
            EXPECT_TRUE(mir_connection_is_valid(connection));

            // Use the number of allowed open files to ensure we exercise the system properly
            struct rlimit rlim;
            getrlimit(RLIMIT_NOFILE, &rlim);
            rlim_t fd_limit = rlim.rlim_cur;
            if (fd_limit == RLIM_INFINITY) fd_limit = 1024;

            for (rlim_t i = 0; i < fd_limit; ++i)
            {
                MirSurfaceParameters request_params =
                {
                    __PRETTY_FUNCTION__,
                    640, 480,
                    mir_pixel_format_abgr_8888,
                    mir_buffer_usage_hardware,
                    mir_display_output_id_invalid
                };

                auto const surface = mir_connection_create_surface_sync(connection, &request_params);
                mir_surface_release_sync(surface);
            }

            mir_connection_release(connection);
            connection_released.raise();
        }}.detach();

    EXPECT_TRUE(connection_released.wait_for(std::chrono::seconds{60}))
        << "Client hung, possibly because of fd leaks" << std::endl;
}
