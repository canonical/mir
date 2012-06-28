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

#include "mir/compositor/buffer_manager.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include <cassert>

namespace mc = mir::compositor;

mc::BufferManager::BufferManager(GraphicBufferAllocator* gr_allocator)
    :
    gr_allocator(gr_allocator),
    client_counter(1)
{
    assert(gr_allocator);
}


void mc::BufferManager::bind_buffer_to_texture()
{

}
 
mc::SurfaceToken mc::BufferManager::create_client(uint32_t width,
                                   uint32_t height,
                                   mc::PixelFormat pf)
{ 
    int new_token = atomic_fetch_add( &client_counter, 1); 
    SurfaceToken token(new_token);

    gr_allocator->alloc_buffer(width, height, pf);

    return token;
}

bool mc::BufferManager::register_buffer(std::shared_ptr<mc::Buffer> buffer)
{
    assert(buffer);
    return false;
}
