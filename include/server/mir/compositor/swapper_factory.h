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
namespace graphics { class GraphicBufferAllocator; }

namespace compositor
{
class SwapperFactory : public BufferAllocationStrategy
{
public:
    explicit SwapperFactory(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& gr_alloc);
    SwapperFactory(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& gr_alloc,
        int number_of_buffers);

    std::shared_ptr<BufferSwapper> create_swapper_reuse_buffers(graphics::BufferProperties const&,
        std::vector<std::shared_ptr<graphics::Buffer>>&, size_t, SwapperType) const;
    std::shared_ptr<BufferSwapper> create_swapper_new_buffers(
        graphics::BufferProperties& actual_properties, graphics::BufferProperties const& requested_properties, SwapperType) const;

private:
    void change_swapper_size(
        std::vector<std::shared_ptr<graphics::Buffer>>&, size_t const, size_t, graphics::BufferProperties const&) const;

    std::shared_ptr<graphics::GraphicBufferAllocator> const gr_allocator;
    unsigned int const synchronous_number_of_buffers;
    unsigned int const spin_number_of_buffers;
};
}
}

#endif // MIR_COMPOSITOR_SWAPPER_FACTORY_H_
