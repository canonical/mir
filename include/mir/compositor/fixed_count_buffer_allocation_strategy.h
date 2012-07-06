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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_COMPOSITOR_FIXED_COUNT_BUFFER_ALLOCATION_STRATEGY_H_
#define MIR_COMPOSITOR_FIXED_COUNT_BUFFER_ALLOCATION_STRATEGY_H_

#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/geometry/dimensions.h"

namespace mir
{
namespace compositor
{

template<unsigned int count>
class FixedCountBufferAllocationStrategy : public BufferAllocationStrategy
{
public:

    static_assert(
        count >= 2,
        "Buffer bundles with less than 2 buffers are not allowed");

    static const unsigned int buffer_count = count;

    FixedCountBufferAllocationStrategy(std::shared_ptr<GraphicBufferAllocator> gr_alloc) : BufferAllocationStrategy(gr_alloc)
    {
    }

    void allocate_buffers_for_bundle(
        geometry::Width width,
        geometry::Height height,
        PixelFormat pf,
        BufferBundle* bundle)
    {
        for(unsigned int i = 0; i < buffer_count; i++)
        {
            bundle->add_buffer(
                graphic_buffer_allocator()->alloc_buffer(
                    width,
                    height,
                    pf));
        }
    }
};

typedef FixedCountBufferAllocationStrategy<2> DoubleBufferAllocationStrategy;
typedef FixedCountBufferAllocationStrategy<3> TripleBufferAllocationStrategy;

}
}

#endif // MIR_COMPOSITOR_FIXED_COUNT_BUFFER_ALLOCATION_STRATEGY_H_
