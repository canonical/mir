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

#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/signal.h"
#include "mir/test/auto_unblock_thread.h"

#include "mir_toolkit/mir_client_library.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <list>
#include <sys/types.h>
#include <dirent.h>

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;

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

namespace
{
unsigned count_fds()
{
    unsigned count = 0;
    DIR* dir;
    EXPECT_TRUE(dir = opendir("/proc/self/fd/"));
    while (readdir(dir) != nullptr)
        count++;
    closedir(dir);
    return count;
}
}

// Regression test for https://bugs.launchpad.net/mir/+bug/1395762
TEST_F(DemoInProcessServerWithStubClientPlatform, surface_creation_does_not_leak_fds)
{
    mir::test::Signal connection_released;

    int fd_count_after_one_surface_lifetime = 0;
               
    mir::test::AutoJoinThread t{
        [&]
        {
            auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
            EXPECT_TRUE(mir_connection_is_valid(connection));

             for (int i = 0; i < 16; ++i)
             {
                auto const surface = mtf::make_any_surface(connection);

                EXPECT_TRUE(mir_surface_is_valid(surface));
                mir_surface_release_sync(surface);

                if (i == 0)
                {
                    fd_count_after_one_surface_lifetime = count_fds();
                }
            }

            mir_connection_release(connection);

            connection_released.raise();

        }};
    

    EXPECT_TRUE(connection_released.wait_for(std::chrono::seconds{480}))
        << "Client hung" << std::endl;

    // Investigation revealed we leak a differing number of fds (3, 0) on Mesa/Android over the
    // entire lifetime of the client library. So we verify here only that we don't leak any FDs beyond
    // those created up to the lifetime of the first surface.
    auto new_fd_count = count_fds();

    EXPECT_LE(new_fd_count, fd_count_after_one_surface_lifetime);

}
