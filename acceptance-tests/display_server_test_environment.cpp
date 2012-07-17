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

#include "display_server_test_environment.h"

#include "mir/display_server.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/renderer.h"

#include <gmock/gmock.h>

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
        return nullptr;
    }
};

}

int test_exit()
{
    return ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
}

std::shared_ptr<mc::BufferAllocationStrategy> DisplayServerTestEnvironment::makeBufferAllocationStrategy()
{
    return std::make_shared<StubBufferAllocationStrategy>();
}

std::shared_ptr<mg::Renderer> DisplayServerTestEnvironment::makeRenderer()
{
    std::shared_ptr < mg::Renderer > renderer =
            std::make_shared<StubRenderer>();
    return renderer;
}

void DisplayServerTestEnvironment::startServer(std::function<void()>&& functor)
{
    auto run_display_server = [&]() -> void
    {
        SCOPED_TRACE("Server");
        server = std::unique_ptr<mir::DisplayServer>(
                new mir::DisplayServer(
                        makeBufferAllocationStrategy(),
                        makeRenderer()));
        functor();
        server->start();
    };
    server_process = mp::fork_and_run_in_a_different_process(run_display_server,
            test_exit);
}

void DisplayServerTestEnvironment::SetUp() 
{
    startServer([]() -> void {} );
}

namespace
{
::testing::AssertionResult IsStarted(
    std::shared_ptr<mir::process::Process> const& server_process)
{
  if (server_process.get() != nullptr)
    return ::testing::AssertionSuccess() << "server started";
  else
    return ::testing::AssertionFailure() << "server NOT started";
}
}

void DisplayServerTestEnvironment::TearDown()
{
    using namespace testing;

    ASSERT_TRUE(IsStarted(server_process));

    server_process->terminate();
    mp::Result const result = server_process->wait_for_termination();

    EXPECT_TRUE(result.signalled());
}

mir::DisplayServer* DisplayServerTestEnvironment::displayServer() const
{
    return server.get();
}

DisplayServerTestEnvironment::DisplayServerTestEnvironment() {}
DisplayServerTestEnvironment::~DisplayServerTestEnvironment() {}
