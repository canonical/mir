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
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/display_server.h"
#include "mir/server_configuration.h"

#include "mir/compositor/compositor.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/renderer.h"
#include "mir/surfaces/surface_stack.h"

#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;

struct mir::DisplayServer::Private
{
    Private(
        const std::shared_ptr<mf::Communicator>& communicator,
        const std::shared_ptr<mc::BufferAllocationStrategy>& strategy,
        const std::shared_ptr<mg::Renderer>& renderer)
            : communicator(communicator),
              buffer_bundle_manager(strategy),
              surface_stack(&buffer_bundle_manager),
              compositor(&surface_stack, renderer),
              exit(false)
    {
    }
    std::shared_ptr<frontend::Communicator> communicator;
    compositor::BufferBundleManager buffer_bundle_manager;
    surfaces::SurfaceStack surface_stack;
    compositor::Compositor compositor;
    std::atomic<bool> exit;
};

mir::DisplayServer::DisplayServer(ServerConfiguration& config) :
    p()
{
    auto buffer_allocation_strategy = config.make_buffer_allocation_strategy();

    p.reset(new mir::DisplayServer::Private(
        config.make_communicator(buffer_allocation_strategy),
        buffer_allocation_strategy,
        config.make_renderer()));
}

mir::DisplayServer::~DisplayServer()
{
}

void mir::DisplayServer::start()
{
    p->communicator->start();
    while (!p->exit.load(std::memory_order_seq_cst))
        do_stuff();
}

void mir::DisplayServer::do_stuff()
{
    //TODO
    std::this_thread::yield();
}

void mir::DisplayServer::stop()
{
    p->exit.store(true, std::memory_order_seq_cst);
}

void mir::DisplayServer::render(mg::Display* display)
{
    p->compositor.render(display);
}
