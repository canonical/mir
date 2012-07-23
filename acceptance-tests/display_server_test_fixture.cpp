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

#include "display_server_test_fixture.h"

#include "mir/display_server.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/renderer.h"

#include <gmock/gmock.h>

#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mp = mir::process;
namespace geom = mir::geometry;
namespace mg = mir::graphics;

namespace
{
class StubRenderer : public mg::Renderer
{
public:
    virtual void render(mg::Renderable&)
    {
    }
};

class StubBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
public:
    virtual std::unique_ptr<mc::BufferSwapper> create_swapper(
        geom::Width, geom::Height, mc::PixelFormat)
    {
        return std::unique_ptr<mc::BufferSwapper>();
    }
};

::testing::AssertionResult WasStarted(
    std::shared_ptr<mir::process::Process> const& server_process)
{
  if (server_process)
    return ::testing::AssertionSuccess() << "server started";
  else
    return ::testing::AssertionFailure() << "server NOT started";
}
}

std::shared_ptr<mc::BufferAllocationStrategy> mir::DisplayServerTestFixture::make_buffer_allocation_strategy()
{
    return std::make_shared<StubBufferAllocationStrategy>();
}

std::shared_ptr<mg::Renderer> mir::DisplayServerTestFixture::make_renderer()
{
    std::shared_ptr < mg::Renderer > renderer =
            std::make_shared<StubRenderer>();
    return renderer;
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


void mir::DisplayServerTestFixture::launch_server_process(std::function<void()>&& functor)
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
                        make_buffer_allocation_strategy(),
                        make_renderer()));

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
        //TODO 4.4 port std::this_thread::sleep_for(std::chrono::milliseconds(20));
        sleep(0);
    }
}


void DisplayServerTestFixture::launch_client_process(std::function<void()>&& functor)
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

void DisplayServerTestFixture::SetUp() 
{
}

void DisplayServerTestFixture::TearDown()
{
    if (server)
    {
        // We're in the server process, so just close down gracefully
        server->stop();
    }

    if (is_test_process)
    {
        using namespace testing;

        for(auto client = clients.begin(); client != clients.end(); ++client)
        {
            EXPECT_TRUE((*client)->wait_for_termination().succeeded());
        }

        // We're in the test process, so make sure we started a service
        ASSERT_TRUE(WasStarted(server_process));
        server_process->terminate();
        mp::Result const result = server_process->wait_for_termination();
        EXPECT_TRUE(result.succeeded());
    }
}

mir::DisplayServer* DisplayServerTestFixture::display_server() const
{
    return server.get();
}

DisplayServerTestFixture::DisplayServerTestFixture() :
    is_test_process(true)
{
}

DisplayServerTestFixture::~DisplayServerTestFixture() {}
