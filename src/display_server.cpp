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

#include "mir/compositor/compositor.h"
#include "mir/compositor/fixed_count_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/graphics/surface_renderer.h"
#include "mir/surfaces/surface_stack.h"

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace mg = mir::graphics;

struct mir::DisplayServer::Private
{
    Private(
        const std::shared_ptr<mc::BufferAllocationStrategy>& strategy,
        const std::shared_ptr<mg::SurfaceRenderer>& renderer)
            : buffer_bundle_manager(strategy),
              surface_stack(&buffer_bundle_manager),
              compositor(&surface_stack, renderer)
    {
    }
    
    compositor::BufferBundleManager buffer_bundle_manager;
    surfaces::SurfaceStack surface_stack;
    compositor::Compositor compositor;
};

mir::DisplayServer::DisplayServer(
    const std::shared_ptr<mc::BufferAllocationStrategy>& strategy,
    const std::shared_ptr<mg::SurfaceRenderer>& renderer) : p(new mir::DisplayServer::Private(strategy, renderer))
{
}

mir::DisplayServer::~DisplayServer()
{
}

void mir::DisplayServer::render(mg::Display* display)
{
    p->compositor.render(display);
}
