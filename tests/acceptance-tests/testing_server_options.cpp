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

#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/renderer.h"
#include "mir/geometry/dimensions.h"
#include "mir/compositor/buffer_swapper.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
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

class StubCommunicator : public mf::Communicator
{
public:
    StubCommunicator() {}
};
}


std::shared_ptr<mc::BufferAllocationStrategy> mir::TestingServerConfiguration::make_buffer_allocation_strategy()
{
    return std::make_shared<StubBufferAllocationStrategy>();
}

void mir::TestingServerConfiguration::exec(DisplayServer* )
{
}

std::shared_ptr<mg::Renderer> mir::TestingServerConfiguration::make_renderer()
{
    return std::make_shared<StubRenderer>();
}

std::shared_ptr<mf::Communicator> mir::TestingServerConfiguration::make_communicator()
{
    return std::make_shared<StubCommunicator>();
}

