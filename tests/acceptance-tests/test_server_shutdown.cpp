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

namespace mt = mir::test;
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

TEST_F(ServerShutdown, normal_exit_removes_endpoint)
{
    run_in_server([]{});

    if (is_test_process())
    {
        ASSERT_TRUE(file_exists(mir_test_socket));

        stop_server();
        EXPECT_FALSE(file_exists(mir_test_socket));
    }
}

using ServerShutdownDeathTest = ServerShutdown;

TEST_F(ServerShutdownDeathTest, abort_removes_endpoint)
{
    mt::CrossProcessSync sync;

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

TEST_F(ServerShutdownDeathTest, fatal_error_abort_causes_abort_on_fatal_error)
{
    // Change the fatal error strategy before starting the Mir server
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};

    mt::CrossProcessSync sync;

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

TEST_F(ServerShutdownDeathTest, fatal_error_abort_removes_endpoint)
{   // Even fatal errors sometimes need to be caught for critical cleanup...
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};

    mt::CrossProcessSync sync;

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

TEST_F(ServerShutdownDeathTest, on_fatal_error_abort_option_causes_abort_on_fatal_error)
{
    add_to_environment( "MIR_SERVER_ON_FATAL_ERROR_ABORT", "");

    mt::CrossProcessSync sync;

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

TEST_F(ServerShutdownDeathTest, mir_fatal_error_during_init_removes_endpoint)
{   // Even fatal errors sometimes need to be caught for critical cleanup...

    add_to_environment("MIR_SERVER_FILE", mir_test_socket);
    server.add_init_callback([&] { mir::fatal_error("Bang"); });
    server.apply_settings();

    mt::CrossProcessSync sync;

    if (auto const pid = fork())
    {
        auto const server_process = std::make_shared<mtf::Process>(pid);

        sync.signal_ready();

        auto result = server_process->wait_for_termination();

        EXPECT_EQ(mtf::TerminationReason::child_terminated_normally, result.reason);
        EXPECT_FALSE(file_exists(mir_test_socket));
    }
    else
    {
        sync.wait_for_signal_ready_for();
        server.run();
    }
}

struct OnSignalDeathTest : ServerShutdown, ::testing::WithParamInterface<int> {};

TEST_P(OnSignalDeathTest, removes_endpoint)
{
    mt::CrossProcessSync sync;

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
    OnSignalDeathTest,
    ::testing::Values(SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS));

