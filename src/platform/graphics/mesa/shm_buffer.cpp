/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "shm_file.h"
#include "shm_buffer.h"
#include "buffer_texture_binder.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;

mgm::ShmBuffer::ShmBuffer(
    std::shared_ptr<ShmFile> const& shm_file,
    geom::Size const& size,
    MirPixelFormat const& pixel_format)
    : shm_file{shm_file},
      size_{size},
      pixel_format_{pixel_format},
      stride_{MIR_BYTES_PER_PIXEL(pixel_format_) * size_.width.as_uint32_t()},
      pixels{shm_file->base_ptr()}
{
}

mgm::ShmBuffer::~ShmBuffer() noexcept
{
}

geom::Size mgm::ShmBuffer::size() const
{
    return size_;
}

geom::Stride mgm::ShmBuffer::stride() const
{
    return stride_;
}

MirPixelFormat mgm::ShmBuffer::pixel_format() const
{
    return pixel_format_;
}

void mgm::ShmBuffer::bind_to_texture()
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
                 size_.width.as_int(), size_.height.as_int(),
                 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                 pixels);
}

std::shared_ptr<MirNativeBuffer> mgm::ShmBuffer::native_buffer_handle() const
{
    auto native_buffer = std::make_shared<MirNativeBuffer>();

    native_buffer->fd_items = 1;
    native_buffer->fd[0] = shm_file->fd();
    native_buffer->stride = stride().as_uint32_t();
    native_buffer->flags = 0;

    auto const& dim = size();
    native_buffer->width = dim.width.as_int();
    native_buffer->height = dim.height.as_int();

    return native_buffer;
}

bool mgm::ShmBuffer::can_bypass() const
{
    return false;
}
