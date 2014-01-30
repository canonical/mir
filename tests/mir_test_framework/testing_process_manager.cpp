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

#include "mir_toolkit/client_types.h"
#include "mir_test_framework/testing_process_manager.h"
#include "mir_test_framework/detect_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "src/client/mir_connection.h"
#include "mir/run_mir.h"

#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace mo = mir::options;
namespace mc = mir::compositor;
namespace mtf = mir_test_framework;

mtf::TestingProcessManager::TestingProcessManager() :
    is_test_process(true),
    server_process_was_started(false)
{
    // In case an earlier test left a stray file
    std::remove(test_socket_file().c_str());
}

mtf::TestingProcessManager::~TestingProcessManager()
{
}

void mtf::TestingProcessManager::launch_server_process(TestingServerConfiguration& config)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        using namespace mir;
        is_test_process = false;

        // We're in the server process, so create a display server
        SCOPED_TRACE("Server");

        mir::run_mir(config, [&](mir::DisplayServer&) { config.exec(); });

        config.on_exit();
    }
    else
    {
        server_process = std::shared_ptr<Process>(new Process(pid));
        config.wait_for_server_start();
        server_process_was_started = true;
    }
}

void mtf::TestingProcessManager::launch_client_process(TestingClientConfiguration& config, mo::Option const& test_options)
{
    if (!is_test_process)
    {
        return; // We're not in the test process, so just return gracefully
    }

    // We're in the test process, so make sure we started a service
    ASSERT_TRUE(server_process_was_started);

    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        is_test_process = false;

        // Need to avoid terminating server or other clients
        server_process->detach();
        for(auto client = clients.begin(); client != clients.end(); ++client)
        {
            (*client)->detach();
        }

        clients.clear();
        server_process.reset();

        SCOPED_TRACE("Client");
        if (!config.use_real_graphics(test_options))
        {
            mtf::UsingStubClientPlatform p;
            config.exec();
        }
        else
        {
            config.exec();
        }

        exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    else
    {
        clients.push_back(std::shared_ptr<Process>(new Process(pid)));
    }
}

void mtf::TestingProcessManager::tear_down_clients()
{
    if (is_test_process)
    {
        for(auto client = clients.begin(); client != clients.end(); ++client)
        {
            auto result((*client)->wait_for_termination());
            EXPECT_TRUE(result.succeeded()) << "client terminate error=" << result;
        }

        clients.clear();
    }
    else
    {
        // Exiting here in the child processes causes "memory leaks",
        // however, not exiting allows the client processes to continue
        // executing tests (which spawn further child processes)
        // with worse results.
        exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
    }
}

mtf::Result mtf::TestingProcessManager::shutdown_server_process()
{
    Result result;

    if (server_process)
    {
        server_process->terminate();
        result = server_process->wait_for_termination();
        server_process.reset();
        return result;
    }
    else
    {
        result.reason = TerminationReason::child_terminated_normally;
        result.exit_code = EXIT_SUCCESS;
    }

    return result;
}

mtf::Result mtf::TestingProcessManager::wait_for_shutdown_server_process()
{
    Result result;

    if (server_process)
    {
        result = server_process->wait_for_termination();
        server_process.reset();
        return result;
    }
    else
    {
        result.reason = TerminationReason::child_terminated_normally;
        result.exit_code = EXIT_SUCCESS;
    }

    return result;
}

void mtf::TestingProcessManager::terminate_client_processes()
{
    if (is_test_process)
    {
        for(auto client : clients)
        {
            client->terminate();
        }
    }
}

void mtf::TestingProcessManager::kill_client_processes()
{
    if (is_test_process)
    {
        for(auto client : clients)
        {
            client->kill();
        }

        clients.clear();
    }
}

void mtf::TestingProcessManager::run_in_test_process(std::function<void()> const& run_code)
{
    if (is_test_process)
        run_code();
}

void mtf::TestingProcessManager::tear_down_server()
{
    if (is_test_process)
    {
        ASSERT_TRUE(clients.empty())  << "Clients should be stopped before server";
        // We're in the test process, so make sure we started a service
        ASSERT_TRUE(server_process_was_started);

        auto const& result = shutdown_server_process();

        EXPECT_TRUE(result.succeeded()) << result;
    }
}

void mtf::TestingProcessManager::tear_down_all()
{
    tear_down_clients();
    tear_down_server();
}
