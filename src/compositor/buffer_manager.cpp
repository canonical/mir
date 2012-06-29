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
#include "mir/graphics/display.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::BufferManager::BufferManager(GraphicBufferAllocator* gr_allocator)
    :
    gr_allocator(gr_allocator)
{
    assert(gr_allocator);
}


mg::Texture mc::BufferManager::bind_buffer_to_texture(surfaces::surfaces_to_render const& )
{
	return mg::Texture();
}
 
std::shared_ptr<mc::Buffer> mc::BufferManager::create_buffer(uint32_t width,
                                   uint32_t height,
                                   mc::PixelFormat pf)
{
    
    return gr_allocator->alloc_buffer(width, height, pf);
}

bool mc::BufferManager::register_buffer(std::shared_ptr<mc::Buffer> buffer)
{
    assert(buffer);
    return false;
}
