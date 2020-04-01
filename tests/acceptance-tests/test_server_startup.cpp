/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/detect_server.h"

#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fcntl.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mtf = mir_test_framework;

using ServerStartup = mtf::InterprocessClientServerTest;

TEST_F(ServerStartup, creates_endpoint_on_filesystem)
{
    ASSERT_FALSE(mtf::detect_server(mtf::test_socket_file(), std::chrono::milliseconds(0)));

    run_in_server([]{});

    run_in_client([]
        {
            EXPECT_TRUE(mtf::detect_server(mtf::test_socket_file(),
                                           std::chrono::milliseconds(100)));
        });
}

TEST_F(ServerStartup, after_server_sigkilled_can_start_new_instance)
{
    ASSERT_FALSE(mtf::detect_server(mtf::test_socket_file(), std::chrono::milliseconds(0)));

    run_in_server([]{});

    if (is_test_process())
    {
        /* Under valgrind, raise(SIGKILL) results in a memcheck orphan process
         * we kill the server process from the test process instead
         */
        EXPECT_TRUE(sigkill_server_process());

        run_in_server([]{});
    }

    run_in_client([]
        {
            EXPECT_TRUE(mtf::detect_server(mtf::test_socket_file(),
                                           std::chrono::milliseconds(100)));
        });
}

TEST(ServerStartupReliability, DISABLED_can_start_with_low_entropy)
{   // Regression test for LP: #1536662 and LP: #1541188
    using namespace ::testing;

    // Flush the entropy pool
    int fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
    ASSERT_THAT(fd, Ge(0));
    char buf[256];
    while (read(fd, buf, sizeof buf) > 0) {}
    close(fd);

    auto start = std::chrono::high_resolution_clock::now();

    struct Server : mtf::HeadlessInProcessServer
    {
        void TestBody() override {}
    } server;

    EXPECT_NO_THROW(server.SetUp(););

    auto duration = std::chrono::high_resolution_clock::now() - start;
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    EXPECT_THAT(seconds, Lt(15));

    server.TearDown();
}
