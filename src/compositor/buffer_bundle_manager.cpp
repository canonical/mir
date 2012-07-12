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
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"

#include <cassert>
#include <memory>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::BufferBundleManager::BufferBundleManager(
    const std::shared_ptr<BufferAllocationStrategy>& strategy)
        : buffer_allocation_strategy(strategy)
{
    assert(strategy);
}


std::shared_ptr<mc::BufferBundle> mc::BufferBundleManager::create_buffer_bundle(
    geometry::Width width,
    geometry::Height height,
    PixelFormat pf)
{
    auto swapper(buffer_allocation_strategy->create_swapper(
            width,
            height,
            pf));

    BufferBundle* new_bundle_raw = new mc::BufferBundle(std::move(swapper));
    std::shared_ptr<mc::BufferBundle> bundle(new_bundle_raw);

    return bundle;
}
