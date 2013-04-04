/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/buffer_swapper.h"
#include "temporary_buffers.h"

namespace mc=mir::compositor;
namespace geom=mir::geometry;

mc::TemporaryBuffer::TemporaryBuffer(std::shared_ptr<Buffer> const& real_buffer)
    : buffer(real_buffer)
{
}

mc::TemporaryClientBuffer::TemporaryClientBuffer(std::shared_ptr<BufferSwapper> const& buffer_swapper)
    : TemporaryBuffer(buffer_swapper->client_acquire()),
      allocating_swapper(buffer_swapper)
{
}

mc::TemporaryClientBuffer::~TemporaryClientBuffer()
{
    if (auto swapper = allocating_swapper.lock())
        swapper->client_release(buffer);
}

mc::TemporaryCompositorBuffer::TemporaryCompositorBuffer(std::shared_ptr<BufferSwapper> const& buffer_swapper)
    : TemporaryBuffer(buffer_swapper->compositor_acquire()),
      allocating_swapper(buffer_swapper)
{
}

mc::TemporaryCompositorBuffer::~TemporaryCompositorBuffer()
{
    if (auto swapper = allocating_swapper.lock())
        swapper->compositor_release(buffer);
}

geom::Size mc::TemporaryBuffer::size() const
{
    return buffer->size();
}

geom::Stride mc::TemporaryBuffer::stride() const
{
    return buffer->stride();
}

geom::PixelFormat mc::TemporaryBuffer::pixel_format() const
{
    return buffer->pixel_format();
}

mc::BufferID mc::TemporaryBuffer::id() const
{
    return buffer->id();
}

void mc::TemporaryBuffer::bind_to_texture()
{
    buffer->bind_to_texture();
}

std::shared_ptr<mc::BufferIPCPackage> mc::TemporaryBuffer::get_ipc_package() const
{
    return buffer->get_ipc_package();
}

