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

#include "mir/compositor/swapper_factory.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/buffer_id.h"
#include "mir/geometry/dimensions.h"

#include <initializer_list>
#include <vector>
#include <cassert>

namespace mc = mir::compositor;

/* todo: once we move to gcc 4.7 it would be slightly better to have a delegated constructor */
mc::SwapperFactory::SwapperFactory(
    std::shared_ptr<GraphicBufferAllocator> const& gr_alloc) :
    gr_allocator(gr_alloc),
    default_number_of_buffers(2)
{
    assert(gr_alloc);
}

mc::SwapperFactory::SwapperFactory(
    std::shared_ptr<GraphicBufferAllocator> const& gr_alloc, int default_number_of_buffers) :
    gr_allocator(gr_alloc),
    default_number_of_buffers(default_number_of_buffers)
{
    assert(gr_alloc);
}

std::unique_ptr<mc::BufferSwapper> mc::SwapperFactory::create_swapper(
    BufferProperties& actual_buffer_properties,
    BufferProperties const& requested_buffer_properties)
{
    auto buf1 = gr_allocator->alloc_buffer(requested_buffer_properties);
    auto buf2 = gr_allocator->alloc_buffer(requested_buffer_properties);

    actual_buffer_properties = BufferProperties{buf1->size(), buf1->pixel_format(), requested_buffer_properties.usage};

    if (default_number_of_buffers == 2)
    {
        return std::unique_ptr<BufferSwapper>(new mc::BufferSwapperMulti({buf1, buf2}));
    } else 
    {
        auto buf3 = gr_allocator->alloc_buffer(requested_buffer_properties);
        return std::unique_ptr<BufferSwapper>(new mc::BufferSwapperMulti({buf1, buf2, buf3}));
    }
}
