/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/buffer_bundle_surfaces.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_id.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"

#include <cassert>
#include <memory>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;

mc::BufferBundleManager::BufferBundleManager(
    const std::shared_ptr<BufferAllocationStrategy>& strategy)
        : buffer_allocation_strategy(strategy)
{
    assert(strategy);
}


std::shared_ptr<ms::BufferBundle> mc::BufferBundleManager::create_buffer_bundle(
    mc::BufferProperties const& buffer_properties)
{
    BufferProperties actual_buffer_properties;

    auto swapper(buffer_allocation_strategy->create_swapper(actual_buffer_properties,
                                                            buffer_properties));

    return std::make_shared<mc::BufferBundleSurfaces>(
        std::move(swapper),
        actual_buffer_properties);
}
