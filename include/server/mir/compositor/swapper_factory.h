/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_COMPOSITOR_SWAPPER_FACTORY_H_
#define MIR_COMPOSITOR_SWAPPER_FACTORY_H_

#include "mir/compositor/buffer_allocation_strategy.h"

namespace mir
{
namespace compositor
{

class GraphicBufferAllocator;
class SwapperAllocator;
struct BufferProperties;

class SwapperFactory : public BufferAllocationStrategy
{
public:
    SwapperFactory(
        std::shared_ptr<GraphicBufferAllocator> const& gr_alloc,
        std::shared_ptr<SwapperAllocator> const& sw_alloc,
        int number_of_buffers = 2);

    std::shared_ptr<BufferSwapper> create_async_swapper_reuse(
        std::vector<std::shared_ptr<Buffer>>&, size_t) const;
    std::shared_ptr<BufferSwapper> create_sync_swapper_reuse(
        std::vector<std::shared_ptr<Buffer>>&, size_t) const;

    std::shared_ptr<BufferSwapper> create_async_swapper_new_buffers(
        BufferProperties&, BufferProperties const&) const;
    std::shared_ptr<BufferSwapper> create_sync_swapper_new_buffers(
        BufferProperties&, BufferProperties const&) const;
private:
    std::shared_ptr<GraphicBufferAllocator> const gr_allocator;
    std::shared_ptr<SwapperAllocator> const swapper_allocator;
    int const number_of_buffers;
};
}
}

#endif // MIR_COMPOSITOR_SWAPPER_FACTORY_H_
