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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "testing_process_manager.h"

#include "mir/display_server.h"

#include <gmock/gmock.h>

#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mp = mir::process;

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

mir::DisplayServer* signal_display_server;

extern "C"
{
void (*signal_prev_fn)(int);

void signal_terminate (int param)
{
    if (SIGTERM == param) signal_display_server->stop();
    else signal_prev_fn(param);
}
}

mir::TestingProcessManager::TestingProcessManager() :
    is_test_process(true)
{
}

mir::TestingProcessManager::~TestingProcessManager()
{
}

void mir::TestingProcessManager::launch_server_process(
    std::shared_ptr<mir::graphics::Renderer> const& renderer,
    std::shared_ptr<mir::compositor::BufferAllocationStrategy> const& buffer_allocation_strategy,
    std::function<void()>&& functor)
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
        server = std::unique_ptr<mir::DisplayServer>(
                new mir::DisplayServer(
                        buffer_allocation_strategy,
                        renderer));

        signal_display_server = server.get();
        signal_prev_fn = signal (SIGTERM, signal_terminate);
        {
            struct ScopedFuture
            {
                std::future<void> future;
                ~ScopedFuture() { future.wait(); }
            } scoped;

            scoped.future = std::async(std::launch::async, std::bind(&mir::DisplayServer::start, server.get()));

            functor();
        }
    }
    else
    {
        server_process = std::shared_ptr<mp::Process>(new mp::Process(pid));
        // A small delay to let the display server get started.
        // TODO there should be a way the server announces "ready"
#ifndef MIR_USING_BOOST_THREADS
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
#else
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
#endif
    }
}

void mir::TestingProcessManager::launch_client_process(std::function<void()>&& functor)
{
    if (!is_test_process)
    {
        clients.clear();
        return; // We're not in the test process, so just return gracefully
    }

    // We're in the test process, so make sure we started a service
    ASSERT_TRUE(WasStarted(server_process));

    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        is_test_process = false;
        SCOPED_TRACE("Client");
        functor();
        exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    else
    {
        clients.push_back(std::shared_ptr<mp::Process>(new mp::Process(pid)));
    }
}

void mir::TestingProcessManager::tear_down_clients()
{
    if (is_test_process)
    {
        using namespace testing;

        for(auto client = clients.begin(); client != clients.end(); ++client)
        {
            EXPECT_TRUE((*client)->wait_for_termination().succeeded());
        }

        clients.clear();
    }
    else
    {
        exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
    }
}

void mir::TestingProcessManager::tear_down_server()
{
    if (is_test_process)
    {
        ASSERT_TRUE(clients.empty())  << "Clients should be stopped before server";
        // We're in the test process, so make sure we started a service
        ASSERT_TRUE(WasStarted(server_process));
        server_process->terminate();
        mp::Result const result = server_process->wait_for_termination();
        EXPECT_TRUE(result.succeeded());
        server_process.reset();
    }
}

void mir::TestingProcessManager::tear_down_all()
{
    if (server)
    {
        // We're in the server process, so just close down gracefully
        server->stop();
    }

    tear_down_clients();
    tear_down_server();
}

mir::DisplayServer* mir::TestingProcessManager::display_server() const
{
    return server.get();
}
