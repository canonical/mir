/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/fatal.h"
#include "mir/server.h"

#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir_test_framework/process.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir_test_framework/executable_path.h"
#include "mir/test/doubles/null_logger.h"

#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mtd = mt::doubles;

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

using ServerShutdownWithGraphicsPlatformException = ::testing::TestWithParam<char const*>;

// Regression test for LP: #1528135
TEST_P(ServerShutdownWithGraphicsPlatformException, clean_shutdown_on_exception)
{
    char const* argv = "ServerShutdownWithException";
    mtf::TemporaryEnvironmentValue exception_set("MIR_TEST_FRAMEWORK_THROWING_PLATFORM_EXCEPTIONS", GetParam());
    mtf::TemporaryEnvironmentValue graphics_platform("MIR_SERVER_PLATFORM_GRAPHICS_LIB", mtf::server_platform("graphics-throw.so").c_str());
    mtf::TemporaryEnvironmentValue input_platform("MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str());
    mir::Server server;

    server.add_configuration_option(mtd::logging_opt, mtd::logging_descr, false);
    server.set_command_line_handler([](int, char const* const*){});
    server.set_command_line(0, &argv);
    server.apply_settings();
    server.run();
}

INSTANTIATE_TEST_SUITE_P(
    PlatformExceptions,
    ServerShutdownWithGraphicsPlatformException,
    ::testing::Values(
        "constructor",
        "create_buffer_allocator",
        "create_display",
        "make_ipc_operations",
        "egl_native_display"
    ));

using ServerShutdownDeathTest = ServerShutdown;

TEST_F(ServerShutdownDeathTest, abort_removes_endpoint)
{
    mt::CrossProcessSync sync;

    run_in_server_and_disable_core_dump([&]
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

    run_in_server_and_disable_core_dump([&]
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

    run_in_server_and_disable_core_dump([&]
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

TEST_F(ServerShutdownDeathTest, abort_on_fatal_error)
{
    mt::CrossProcessSync sync;

    run_in_server_and_disable_core_dump([&]
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

    add_to_environment( "MIR_SERVER_ON_FATAL_ERROR_EXCEPT", "");
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

    run_in_server_and_disable_core_dump([&]
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

INSTANTIATE_TEST_SUITE_P(ServerShutdown,
    OnSignalDeathTest,
    ::testing::Values(SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS));

