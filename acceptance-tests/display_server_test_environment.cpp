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
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/renderer.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mp = mir::process;
namespace geom = mir::geometry;
namespace gfx = mir::graphics;

namespace
{
class StubRenderer : public gfx::Renderer
{
public:
    virtual void render(gfx::Renderable&)
    {
    }
};

class StubBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    virtual std::shared_ptr<mc::Buffer> alloc_buffer(
        geom::Width, geom::Height, mc::PixelFormat)
    {
        return nullptr;
    }
};

class StubBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
public:
    StubBufferAllocationStrategy()
        : mc::BufferAllocationStrategy(std::make_shared<StubBufferAllocator>())
    {
    }

    virtual void allocate_buffers_for_bundle(
        geom::Width, geom::Height, mc::PixelFormat, mc::BufferBundle *)
    {
    }
};

}

mp::Process::ExitCode test_exit()
{
    return ::testing::Test::HasFailure() ?
        mp::Process::ExitCode::failure : 
        mp::Process::ExitCode::success;
}

std::shared_ptr<mp::Process> DisplayServerTestEnvironment::display_server_process()
{
    return server;
}

void DisplayServerTestEnvironment::SetUp() 
{
    auto server_bind_and_connect = []() -> void
    {
        SCOPED_TRACE("Server");
        auto  strategy = std::make_shared<StubBufferAllocationStrategy>();
        auto renderer = std::make_shared<StubRenderer>();
        mir::DisplayServer display_server(strategy, renderer);
    };
    server = mp::fork_and_run_in_a_different_process(
        server_bind_and_connect, test_exit);
}

void DisplayServerTestEnvironment::TearDown()
{
    server->terminate();
    EXPECT_TRUE(server->wait_for_termination().is_successful());
}

int main(int argc, char **argv)
{
    ::testing::AddGlobalTestEnvironment(new DisplayServerTestEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
