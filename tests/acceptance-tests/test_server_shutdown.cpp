/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/fatal.h"

#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir_test_framework/process.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

using ServerShutdown = mtf::InterprocessClientServerTest;

namespace
{
bool file_exists(std::string const& filename)
{
    struct stat statbuf;
    return 0 == stat(filename.c_str(), &statbuf);
}
}

TEST_F(ServerShutdown, server_removes_endpoint_on_normal_exit)
{
    run_in_server([]{});

    if (is_test_process())
    {
        ASSERT_TRUE(file_exists(mir_test_socket));

        stop_server();
        EXPECT_FALSE(file_exists(mir_test_socket));
    }
}

TEST_F(ServerShutdown, server_removes_endpoint_on_abort)
{
    mtf::CrossProcessSync sync;

    run_in_server([&]
        {
            sync.wait_for_signal_ready_for();
            abort();
        });

    if (is_test_process())
    {
        ASSERT_TRUE(file_exists(mir_test_socket));

        sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness.
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == SIGABRT || result.signal == SIGKILL);

        EXPECT_FALSE(file_exists(mir_test_socket));
    }
}

TEST_F(ServerShutdown, the_fatal_error_default_can_be_changed_to_abort)
{
    // Change the fatal error strategy before starting the Mir server
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};

    mtf::CrossProcessSync sync;

    run_in_server([&]
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        });

    if (is_test_process())
    {
        sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness.
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == SIGABRT || result.signal == SIGKILL);
    }
}

TEST_F(ServerShutdown, server_removes_endpoint_on_mir_fatal_error_abort)
{   // Even fatal errors sometimes need to be caught for critical cleanup...
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};

    mtf::CrossProcessSync sync;

    run_in_server([&]
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        });

    if (is_test_process())
    {
        ASSERT_TRUE(file_exists(mir_test_socket));

        sync.signal_ready();

        wait_for_shutdown_server_process();

        EXPECT_FALSE(file_exists(mir_test_socket));
    };
}

TEST_F(ServerShutdown, setting_on_fatal_error_abort_option_causes_abort_on_fatal_error)
{
    add_to_environment( "MIR_SERVER_ON_FATAL_ERROR_ABORT", "");

    mtf::CrossProcessSync sync;

    run_in_server([&]
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        });

    if (is_test_process())
    {
        sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness.
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == SIGABRT || result.signal == SIGKILL);
    }
}

#if 0
// TODO There's no easy way to inject the fatal error with the new API test fixtures.
// TODO It can be placed into server.add_init_callback() but then prevents the
// TODO server actually starting the main loop and signalling that the test process
// TODO can continue.
// TODO I'm not convinced it adds enough value to justify reworking it
TEST_F(ServerShutdown, server_removes_endpoint_on_mir_fatal_error_except)
{   // Even fatal errors sometimes need to be caught for critical cleanup...
    mtf::CrossProcessSync sync;

    run_in_server([&]
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        });

    if (is_test_process())
    {
        ASSERT_TRUE(file_exists(server_config.the_socket_file()));

        server_config.sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_normally, result.reason);

        EXPECT_FALSE(file_exists(server_config.the_socket_file()));
    });
}

// Here's a failing attempt...
TEST_F(ServerShutdown, server_removes_endpoint_on_mir_fatal_error_except)
{   // Even fatal errors sometimes need to be caught for critical cleanup...
    mtf::CrossProcessSync sync;

    run_in_server([&]
        {
            sync.wait_for_signal_ready_for();
            server.the_main_loop()->enqueue(this, [&] { mir::fatal_error("Bang"); });
        });

    if (is_test_process())
    {
        ASSERT_TRUE(file_exists(mir_test_socket));

        sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_normally, result.reason);

        EXPECT_FALSE(file_exists(mir_test_socket));
    }
}
#endif

struct OnSignal : ServerShutdown, ::testing::WithParamInterface<int> {};

TEST_P(OnSignal, removes_endpoint_on_signal)
{
    mtf::CrossProcessSync sync;

    run_in_server([&]
        {
            sync.wait_for_signal_ready_for();
            raise(GetParam());
        });

    if (is_test_process())
    {
        ASSERT_TRUE(file_exists(mir_test_socket));

        sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == GetParam() || result.signal == SIGKILL);

        EXPECT_FALSE(file_exists(mir_test_socket));
    }
}

INSTANTIATE_TEST_CASE_P(ServerShutdown,
    OnSignal,
    ::testing::Values(SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS));
