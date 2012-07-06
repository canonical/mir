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
 *  Alan Griffiths <alan@octopull.co.uk>
 *  Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_
#define MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_

#include "mir/compositor/buffer.h"
#include "mir/geometry/dimensions.h"

#include <cassert>
#include <memory>

namespace mir
{
namespace compositor
{

class GraphicBufferAllocator;
class BufferBundle;

class BufferAllocationStrategy
{
 public:
    virtual ~BufferAllocationStrategy() {}

    virtual void allocate_buffers_for_bundle(
        geometry::Width width,
        geometry::Height height,
        PixelFormat pf,
        BufferBundle* bundle) = 0;
    
 protected:
    BufferAllocationStrategy(std::shared_ptr<GraphicBufferAllocator> allocator) : gr_allocator(allocator)
    {
        assert(allocator);
    }
    BufferAllocationStrategy(const BufferAllocationStrategy&);
    BufferAllocationStrategy& operator=(const BufferAllocationStrategy& );

    std::shared_ptr<GraphicBufferAllocator> graphic_buffer_allocator()
    {
        return gr_allocator;
    }
    
 private:
    std::shared_ptr<GraphicBufferAllocator> gr_allocator;
};

}
}

#endif // MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_
