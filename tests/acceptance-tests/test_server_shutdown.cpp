/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

using ServerShutdown = BespokeDisplayServerTestFixture;

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
    using ServerConfig = TestingServerConfiguration;

    ServerConfig server_config;
    launch_server_process(server_config);

    run_in_test_process([&]
    {
        ASSERT_TRUE(file_exists(server_config.the_socket_file()));

        shutdown_server_process();
        EXPECT_FALSE(file_exists(server_config.the_socket_file()));
    });
}

TEST_F(ServerShutdown, server_removes_endpoint_on_abort)
{
    struct ServerConfig : TestingServerConfiguration
    {
        void on_start() override
        {
            sync.wait_for_signal_ready_for();
            abort();
        }

        mtf::CrossProcessSync sync;
    };

    ServerConfig server_config;
    launch_server_process(server_config);

    run_in_test_process([&]
    {
        ASSERT_TRUE(file_exists(server_config.the_socket_file()));

        server_config.sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness.
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == SIGABRT || result.signal == SIGKILL);

        EXPECT_FALSE(file_exists(server_config.the_socket_file()));
    });
}

TEST_F(ServerShutdown, the_fatal_error_default_can_be_changed_to_abort)
{
    // Change the fatal error strategy before starting the Mir server
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};

    struct ServerConfig : TestingServerConfiguration
    {
        void on_start() override
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        }

        mtf::CrossProcessSync sync;
    };

    ServerConfig server_config;
    launch_server_process(server_config);

    run_in_test_process([&]
    {
        server_config.sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness.
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == SIGABRT || result.signal == SIGKILL);
    });
}

TEST_F(ServerShutdown, server_removes_endpoint_on_mir_fatal_error_abort)
{   // Even fatal errors sometimes need to be caught for critical cleanup...
    struct ServerConfig : TestingServerConfiguration
    {
        void on_start() override
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        }

        mir::FatalErrorStrategy on_error{mir::fatal_error_abort};
        mtf::CrossProcessSync sync;
    };

    ServerConfig server_config;
    launch_server_process(server_config);

    run_in_test_process([&]
    {
        ASSERT_TRUE(file_exists(server_config.the_socket_file()));

        server_config.sync.signal_ready();

        wait_for_shutdown_server_process();

        EXPECT_FALSE(file_exists(server_config.the_socket_file()));
    });
}

TEST_F(ServerShutdown, setting_on_fatal_error_abort_option_causes_abort_on_fatal_error)
{
    static auto const env_on_fatal_error_abort = "MIR_SERVER_ON_FATAL_ERROR_ABORT";
    auto const old_env(getenv(env_on_fatal_error_abort));
    if (!old_env) setenv(env_on_fatal_error_abort, "", true);

    struct ServerConfig : TestingServerConfiguration
    {
        void on_start() override
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        }

        mtf::CrossProcessSync sync;
    };

    ServerConfig server_config;
    launch_server_process(server_config);

    run_in_test_process([&]
    {
        server_config.sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness.
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == SIGABRT || result.signal == SIGKILL);
    });

    if (!old_env) unsetenv(env_on_fatal_error_abort);
}

TEST_F(ServerShutdown, server_removes_endpoint_on_mir_fatal_error_except)
{   // Even fatal errors sometimes need to be caught for critical cleanup...
    struct ServerConfig : TestingServerConfiguration
    {
        void on_start() override
        {
            sync.wait_for_signal_ready_for();
            mir::fatal_error("Bang");
        }

        mtf::CrossProcessSync sync;
    };

    ServerConfig server_config;
    launch_server_process(server_config);

    run_in_test_process([&]
    {
        ASSERT_TRUE(file_exists(server_config.the_socket_file()));

        server_config.sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_normally, result.reason);

        EXPECT_FALSE(file_exists(server_config.the_socket_file()));
    });
}

struct OnSignal : ServerShutdown, ::testing::WithParamInterface<int> {};

TEST_P(OnSignal, removes_endpoint_on_signal)
{
    struct ServerConfig : TestingServerConfiguration
    {
        void on_start() override
        {
            sync.wait_for_signal_ready_for();
            raise(sig);
        }

        ServerConfig(int sig) : sig(sig) {}

        int const sig;
        mtf::CrossProcessSync sync;
    };

    ServerConfig server_config(GetParam());
    launch_server_process(server_config);

    run_in_test_process([&]
    {
        ASSERT_TRUE(file_exists(server_config.the_socket_file()));

        server_config.sync.signal_ready();

        auto result = wait_for_shutdown_server_process();
        EXPECT_EQ(mtf::TerminationReason::child_terminated_by_signal, result.reason);
        // Under valgrind the server process is reported as being terminated
        // by SIGKILL because of multithreading madness
        // TODO: Investigate if we can do better than this workaround
        EXPECT_TRUE(result.signal == GetParam() || result.signal == SIGKILL);

        EXPECT_FALSE(file_exists(server_config.the_socket_file()));
    });
}

INSTANTIATE_TEST_CASE_P(ServerShutdown,
    OnSignal,
    ::testing::Values(SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS));

