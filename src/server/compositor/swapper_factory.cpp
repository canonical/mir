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
#include "switching_bundle.h"

#include <initializer_list>
#include <vector>
#include <cassert>

namespace mc = mir::compositor;

mc::SwapperFactory::SwapperFactory(
        std::shared_ptr<GraphicBufferAllocator> const& gr_alloc,
        int number_of_buffers)
    : gr_allocator(gr_alloc),
      synchronous_number_of_buffers(number_of_buffers),
      spin_number_of_buffers(3) //spin algorithm always takes 3 buffers
{
}

mc::SwapperFactory::SwapperFactory(
    std::shared_ptr<GraphicBufferAllocator> const& gr_alloc)
    : SwapperFactory(gr_alloc, 2)
{
}

std::shared_ptr<mc::BufferSwapper> mc::SwapperFactory::create_swapper_reuse_buffers(
    std::vector<std::shared_ptr<Buffer>>& list, size_t buffer_num, SwapperType type) const
{
    printf("realloc!\n");
    if (type == mc::SwapperType::synchronous)
    {
        return std::make_shared<mc::BufferSwapperMulti>(list, buffer_num); 
    }
    else
    {
        while (buffer_num < spin_numebr_of_buffers)
        {

        } 
        return std::make_shared<mc::BufferSwapperSpin>(list, buffer_num);
    }
}

std::shared_ptr<mc::BufferSwapper> mc::SwapperFactory::create_swapper_new_buffers(
    BufferProperties& actual_buffer_properties,
    BufferProperties const& requested_buffer_properties, SwapperType type) const
{
    std::vector<std::shared_ptr<mc::Buffer>> list;
    std::shared_ptr<mc::BufferSwapper> new_swapper;

    if (type == mc::SwapperType::synchronous)
    {
        for(auto i=0; i< number_of_buffers; i++)
        {
            list.push_back(gr_allocator->alloc_buffer(requested_buffer_properties));
        }
        new_swapper = std::make_shared<mc::BufferSwapperMulti>(list, number_of_buffers);
    }
    else
    {
        int const async_buffer_count = 3; //async only can accept 3 buffers, so ignore constructor request
        for(auto i=0; i < async_buffer_count; i++)
        {
            list.push_back(gr_allocator->alloc_buffer(requested_buffer_properties));
        }
        new_swapper = std::make_shared<mc::BufferSwapperSpin>(list, async_buffer_count);
    }

    actual_buffer_properties = BufferProperties{
        list[0]->size(), list[0]->pixel_format(), requested_buffer_properties.usage};
    return new_swapper;
}
