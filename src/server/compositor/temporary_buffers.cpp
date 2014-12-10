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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "buffer_bundle.h"
#include "temporary_buffers.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

mc::TemporaryBuffer::TemporaryBuffer(std::shared_ptr<mg::Buffer> const& real_buffer)
    : buffer(real_buffer)
{
}

mc::TemporaryCompositorBuffer::TemporaryCompositorBuffer(
    std::shared_ptr<BufferBundle> const& bun, void const* user_id)
    : TemporaryBuffer(bun->compositor_acquire(user_id)),
      bundle(bun)
{
}

mc::TemporaryCompositorBuffer::~TemporaryCompositorBuffer()
{
    bundle->compositor_release(buffer);
}

mc::TemporarySnapshotBuffer::TemporarySnapshotBuffer(
    std::shared_ptr<BufferBundle> const& bun)
    : TemporaryBuffer(bun->snapshot_acquire()),
      bundle(bun)
{
}

mc::TemporarySnapshotBuffer::~TemporarySnapshotBuffer()
{
    bundle->snapshot_release(buffer);
}

geom::Size mc::TemporaryBuffer::size() const
{
    return buffer->size();
}

geom::Stride mc::TemporaryBuffer::stride() const
{
    return buffer->stride();
}

MirPixelFormat mc::TemporaryBuffer::pixel_format() const
{
    return buffer->pixel_format();
}

mg::BufferID mc::TemporaryBuffer::id() const
{
    return buffer->id();
}

void mc::TemporaryBuffer::gl_bind_to_texture()
{
    buffer->gl_bind_to_texture();
}

std::shared_ptr<mg::NativeBuffer> mc::TemporaryBuffer::native_buffer_handle() const
{
    return buffer->native_buffer_handle();
}

bool mc::TemporaryBuffer::can_bypass() const
{
    return buffer->can_bypass();
}

void mc::TemporaryBuffer::write(unsigned char const*, size_t)
{
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Write to temporary buffer snapshot is ill advised and indicates programmer error"));
}
