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

#include "testing_process_manager.h"

#include "mir/display_server.h"
#include "mir/process/signal_dispatcher.h"

#include "mir/thread/all.h"

#include <gmock/gmock.h>

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

void startup_pause()
{
#ifndef MIR_USING_BOOST_THREADS
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
#else
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
#endif
}
}

mir::TestingProcessManager::TestingProcessManager() :
    is_test_process(true)
{
}

mir::TestingProcessManager::~TestingProcessManager()
{
}

void mir::TestingProcessManager::launch_server_process(TestingServerConfiguration& config)
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
                        config.make_buffer_allocation_strategy(),
                        config.make_renderer()));

        mp::SignalDispatcher::instance()->enable_for(SIGTERM);
        mp::SignalDispatcher::instance()->signal_channel().connect(
                boost::bind(&TestingProcessManager::os_signal_handler, this, _1));

        struct ScopedFuture
        {
            std::future<void> future;
            ~ScopedFuture() { future.wait(); }
        } scoped;

        scoped.future = std::async(std::launch::async, std::bind(&mir::DisplayServer::start, server.get()));

        config.exec(display_server());
    }
    else
    {
        server_process = std::shared_ptr<mp::Process>(new mp::Process(pid));
        // A small delay to let the display server get started.
        // TODO there should be a way the server announces "ready"
        startup_pause();
    }
}

void mir::TestingProcessManager::launch_client_process(TestingClientConfiguration& config)
{
    if (!is_test_process)
    {
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

        for(auto client = clients.begin(); client != clients.end(); ++client)
        {
            (*client)->detach();
        }

        clients.clear();

        SCOPED_TRACE("Client");
        config.exec();
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
            auto result((*client)->wait_for_termination());
            EXPECT_TRUE(result.succeeded()) << "result=" << result;
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
        startup_pause(); // TODO frig
        mp::Result const result = server_process->wait_for_termination();
        EXPECT_TRUE(result.succeeded()) << result;
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

void mir::TestingProcessManager::os_signal_handler(int signal)
{
    switch(signal)
    {
    case SIGTERM:
        server->stop();
        break;
    default:
        break;
    }
}
