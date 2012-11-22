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

#include "mir_test_framework/testing_process_manager.h"

#include "mir/display_server.h"

#include "mir/chrono/chrono.h"
#include "mir/thread/all.h"

#include <boost/asio.hpp>

#include <gmock/gmock.h>

#include <stdexcept>

namespace mc = mir::compositor;
namespace mp = mir::process;
namespace mtf = mir_test_framework;

namespace
{
::testing::AssertionResult WasStarted(
    std::shared_ptr<mir::process::Process> const& server_process)
{
    if (server_process)
        return ::testing::AssertionSuccess() << "server started";
    else
        return ::testing::AssertionFailure() << "server NOT started";
}
}

namespace mir_test_framework
{
void startup_pause()
{
    if (!detect_server(test_socket_file(), std::chrono::milliseconds(2000)))
        throw std::runtime_error("Failed to find server");
}
}

mtf::TestingProcessManager::TestingProcessManager() :
    is_test_process(true),
    server_process_was_started(false)
{
}

mtf::TestingProcessManager::~TestingProcessManager()
{
}

namespace
{
// TODO: Get rid of the volatile-hack here and replace it with
// some sane atomic-pointer once we have left GCC 4.4 behind.
mir::DisplayServer* volatile signal_display_server;
}

namespace mir
{
extern "C"
{
void (*signal_prev_fn)(int);
void signal_terminate (int )
{
    while (!signal_display_server)
        std::this_thread::yield();
 
    signal_display_server->stop();
}
}
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

        signal_prev_fn = signal(SIGTERM, signal_terminate);

        mir::DisplayServer server(config);

        signal_display_server = &server;

        {
            struct ScopedFuture
            {
                std::future<void> future;
                ~ScopedFuture() { future.wait(); }
            } scoped;

            scoped.future = std::async(std::launch::async, std::bind(&mir::DisplayServer::start, &server));

            config.exec(&server);
        }

        config.on_exit(&server);
    }
    else
    {
        server_process = std::shared_ptr<mp::Process>(new mp::Process(pid));
        startup_pause();
        server_process_was_started = true;
    }
}

void mtf::TestingProcessManager::launch_client_process(TestingClientConfiguration& config)
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
        config.exec();
        exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    else
    {
        clients.push_back(std::shared_ptr<mp::Process>(new mp::Process(pid)));
    }
}

void mtf::TestingProcessManager::tear_down_clients()
{
    if (is_test_process)
    {
        using namespace testing;

        if (clients.empty())
        {
            // Allow some time for server-side only tests to run
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

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

mp::Result mtf::TestingProcessManager::shutdown_server_process()
{
    mp::Result result;

    if (server_process)
    {
        server_process->terminate();
        result = server_process->wait_for_termination();
        server_process.reset();
        return result;
    }
    else
    {
        result.reason = mp::TerminationReason::child_terminated_normally;
        result.exit_code = EXIT_SUCCESS;
    }

    return result;
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

bool mtf::detect_server(
        const std::string& socket_file,
        std::chrono::milliseconds const& timeout)
{
    auto limit = std::chrono::system_clock::now() + timeout;

    bool error = false;
    struct stat file_status;

    do
    {
      if (error) {
          std::this_thread::sleep_for(std::chrono::milliseconds(0));
      }
      error = stat(socket_file.c_str(), &file_status);
    }
    while (error && std::chrono::system_clock::now() < limit);

    struct sockaddr_un remote; 
    auto sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, socket_file.c_str());
    auto len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    do
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }
    while ((connect(sockfd, (struct sockaddr *)&remote, len) == -1)
            && (std::chrono::system_clock::now() < limit));

    close(sockfd);

    return !error;
}

