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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/swapper_factory.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/compositor/buffer_swapper_spin.h"
#include "mir/compositor/buffer_id.h"
#include "mir/geometry/dimensions.h"
#include "swapper_switcher.h"

#include <initializer_list>
#include <vector>
#include <cassert>

namespace mc = mir::compositor;

mc::SwapperFactory::SwapperFactory(
        std::shared_ptr<GraphicBufferAllocator> const& gr_alloc,
        int number_of_buffers)
    : gr_allocator(gr_alloc),
      number_of_buffers(number_of_buffers)
{
    assert(gr_alloc);
}

void mc::SwapperFactory::fill_buffer_list(std::vector<std::shared_ptr<mc::Buffer>>& list, int num_buffers,
                                          BufferProperties const& requested_buffer_properties) const
{
    printf("NUM %i\n", num_buffers);
    for(auto i=0; i< num_buffers; i++)
    {
        list.push_back(
            gr_allocator->alloc_buffer(requested_buffer_properties));
    }
}

std::shared_ptr<mc::BufferSwapper> mc::SwapperFactory::create_sync_swapper_reuse(
    std::vector<std::shared_ptr<Buffer>>& list, size_t buffer_num) const
{
    return std::make_shared<mc::BufferSwapperMulti>(list, buffer_num); 
}

std::shared_ptr<mc::BufferSwapper> mc::SwapperFactory::create_sync_swapper_new_buffers(
    BufferProperties&, BufferProperties const& requested_buffer_properties) const
{
    std::vector<std::shared_ptr<mc::Buffer>> list;
    fill_buffer_list(list, number_of_buffers, requested_buffer_properties);
    return std::make_shared<mc::BufferSwapperMulti>(list, number_of_buffers); 
}

std::shared_ptr<mc::BufferSwapper> mc::SwapperFactory::create_async_swapper_reuse(
    std::vector<std::shared_ptr<Buffer>>& list, size_t buffer_num) const
{
    return std::make_shared<mc::BufferSwapperSpin>(list, buffer_num);; 
}

std::shared_ptr<mc::BufferSwapper> mc::SwapperFactory::create_async_swapper_new_buffers(
    BufferProperties&, BufferProperties const& requested_buffer_properties) const
{
    int const async_buffer_count = 3; //async only can accept 3 buffers
    std::vector<std::shared_ptr<mc::Buffer>> list;
    fill_buffer_list(list, async_buffer_count, requested_buffer_properties);
    return std::make_shared<mc::BufferSwapperSpin>(list, async_buffer_count);; 
}

#if 0 
std::unique_ptr<mc::BufferSwapperMaster> mc::SwapperFactory::create_swapper_master(
    BufferProperties& actual_buffer_properties,
    BufferProperties const& requested_buffer_properties)
{
    std::vector<std::shared_ptr<mc::Buffer>> buffers;

    for(auto i=0; i< number_of_buffers; i++)
    {
        buffers.push_back(
            gr_allocator->alloc_buffer(requested_buffer_properties));
    }

    actual_buffer_properties = BufferProperties{buffers[0]->size(), buffers[0]->pixel_format(), requested_buffer_properties.usage};

    auto swapper = std::make_shared<mc::BufferSwapperMulti>(buffers, number_of_buffers);
    return std::unique_ptr<BufferSwapperMaster>(new mc::SwapperSwitcher(swapper));
}
#endif

