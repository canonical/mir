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

#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/renderer.h"


namespace mc = mir::compositor;
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

void mir::DisplayServerTestFixture::launch_server_process(std::function<void()>&& functor)
{
    process_manager.launch_server_process(
        make_renderer(),
        make_buffer_allocation_strategy(),
        functor);
}

void DisplayServerTestFixture::launch_client_process(std::function<void()>&& functor)
{
    process_manager.launch_client_process(functor);
}

void DisplayServerTestFixture::SetUp()
{
}

void DisplayServerTestFixture::TearDown()
{
    process_manager.tear_down();
}

mir::DisplayServer* DisplayServerTestFixture::display_server() const
{
    return process_manager.display_server();
}

DisplayServerTestFixture::DisplayServerTestFixture() :
    process_manager()
{
}

DisplayServerTestFixture::~DisplayServerTestFixture() {}
