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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/temporary_compositor_buffer.h"

#include <boost/exception/all.hpp>

namespace mc=mir::compositor;
namespace geom=mir::geometry;

mc::TemporaryCompositorBuffer::TemporaryCompositorBuffer(const std::shared_ptr<BufferSwapper>& buffer_swapper)
{
    buffer_swapper->compositor_acquire(buffer, buffer_id);
    allocating_swapper = buffer_swapper; 
}

mc::TemporaryCompositorBuffer::~TemporaryCompositorBuffer()
{
    if (auto swapper = allocating_swapper.lock())
        swapper->compositor_release(buffer_id);
}

geom::Size mc::TemporaryCompositorBuffer::size() const
{
    return buffer->size();
}

geom::Stride mc::TemporaryCompositorBuffer::stride() const
{
    return buffer->stride();
}

geom::PixelFormat mc::TemporaryCompositorBuffer::pixel_format() const
{
    return buffer->pixel_format();
}

mc::BufferID mc::TemporaryCompositorBuffer::id() const
{
    return buffer->id();
}

void mc::TemporaryCompositorBuffer::bind_to_texture()
{
    buffer->bind_to_texture();
}

std::shared_ptr<mc::BufferIPCPackage> mc::TemporaryCompositorBuffer::get_ipc_package() const
{   
    return buffer->get_ipc_package();
}

