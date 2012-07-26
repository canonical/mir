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

#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper_double.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/geometry/dimensions.h"

#include <cassert>

namespace mc = mir::compositor;


mc::DoubleBufferAllocationStrategy::DoubleBufferAllocationStrategy(
    std::shared_ptr<GraphicBufferAllocator> const& gr_alloc) :
    gr_allocator(gr_alloc)
{
    assert(gr_alloc);
}

std::unique_ptr<mc::BufferSwapper> mc::DoubleBufferAllocationStrategy::create_swapper(
    geometry::Width width,
    geometry::Height height,
    PixelFormat pf)
{
    return std::unique_ptr<BufferSwapper>(
        new BufferSwapperDouble(
            gr_allocator->alloc_buffer(width, height, pf),
            gr_allocator->alloc_buffer(width, height, pf)));
}
