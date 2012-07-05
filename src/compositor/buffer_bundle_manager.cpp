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

#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::BufferBundleManager::BufferBundleManager(
    BufferAllocationStrategy* strategy,
    GraphicBufferAllocator* gr_allocator)
        : buffer_allocation_strategy(strategy),
          gr_allocator(gr_allocator)
{
    assert(strategy);
    assert(gr_allocator);
}


std::shared_ptr<mc::BufferBundle> mc::BufferBundleManager::create_buffer_bundle(
    geometry::Width width,
    geometry::Height height,
    PixelFormat pf)
{
    BufferBundle* new_bundle_raw = new mc::BufferBundle(); 
    std::shared_ptr<mc::BufferBundle> bundle(new_bundle_raw);
    bundle_list.push_back(bundle);

    buffer_allocation_strategy->allocate_buffers_for_bundle(
        width,
        height,
        pf,
        new_bundle_raw);
    
    return bundle;
}

void mc::BufferBundleManager::destroy_buffer_bundle(std::shared_ptr<mc::BufferBundle> bundle) {

    bundle_list.remove(bundle);
 
}

bool mc::BufferBundleManager::is_empty() {
    return bundle_list.empty();
}
