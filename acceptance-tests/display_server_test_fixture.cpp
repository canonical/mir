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

void empty_function() {}
}

mir::TestingProcessManager mir::DefaultDisplayServerTestFixture::process_manager;

std::shared_ptr<mc::BufferAllocationStrategy> mir::DefaultDisplayServerTestFixture::make_buffer_allocation_strategy()
{
    return std::make_shared<StubBufferAllocationStrategy>();
}

std::shared_ptr<mg::Renderer> mir::DefaultDisplayServerTestFixture::make_renderer()
{
    return std::make_shared<StubRenderer>();
}

void DefaultDisplayServerTestFixture::launch_client_process(std::function<void()>&& functor)
{
    process_manager.launch_client_process(std::move(functor));
}

void DefaultDisplayServerTestFixture::SetUpTestCase()
{
    process_manager.launch_server_process(
        make_renderer(),
        make_buffer_allocation_strategy(),
        empty_function);
}


void DefaultDisplayServerTestFixture::TearDown()
{
    process_manager.tear_down_clients();
}

void DefaultDisplayServerTestFixture::TearDownTestCase()
{
    process_manager.tear_down_all();
}

mir::DisplayServer* DefaultDisplayServerTestFixture::display_server() const
{
    return process_manager.display_server();
}

DefaultDisplayServerTestFixture::DefaultDisplayServerTestFixture()
{
}

DefaultDisplayServerTestFixture::~DefaultDisplayServerTestFixture() {}


std::shared_ptr<mc::BufferAllocationStrategy> mir::BespokeDisplayServerTestFixture::make_buffer_allocation_strategy()
{
    return std::make_shared<StubBufferAllocationStrategy>();
}

std::shared_ptr<mg::Renderer> mir::BespokeDisplayServerTestFixture::make_renderer()
{
    std::shared_ptr < mg::Renderer > renderer =
            std::make_shared<StubRenderer>();
    return renderer;
}

void mir::BespokeDisplayServerTestFixture::launch_server_process(std::function<void()>&& functor)
{
    process_manager.launch_server_process(
        make_renderer(),
        make_buffer_allocation_strategy(),
        std::move(functor));
}

void BespokeDisplayServerTestFixture::launch_client_process(std::function<void()>&& functor)
{
    process_manager.launch_client_process(std::move(functor));
}

void BespokeDisplayServerTestFixture::SetUp()
{
}

void BespokeDisplayServerTestFixture::TearDown()
{
    process_manager.tear_down_all();
}

mir::DisplayServer* BespokeDisplayServerTestFixture::display_server() const
{
    return process_manager.display_server();
}

BespokeDisplayServerTestFixture::BespokeDisplayServerTestFixture() :
    process_manager()
{
}

BespokeDisplayServerTestFixture::~BespokeDisplayServerTestFixture() {}
