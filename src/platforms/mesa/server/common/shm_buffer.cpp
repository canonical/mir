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

#include <boost/throw_exception.hpp>

#include <stdexcept>

#include <string.h>

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
      pixels{shm_file->base_ptr()},
      gl_format{GL_INVALID_ENUM},
      gl_type{GL_INVALID_ENUM}
{
    bool const little_endian = true;  // TODO big endian platforms
    switch (pixel_format_)
    {
    case mir_pixel_format_rgb_565:
        if (little_endian)
        {
            gl_format = GL_RGB;
            gl_type = GL_UNSIGNED_SHORT_5_6_5;
        }
        break;

    // XXX mir_pixel_format_xrgb_8888: Historically we have always allowed
    //     this, but it will look wrong if the client fails to fill the
    //     X (alpha) bytes with 255.
    //     We could fill it ourselves, but that would impair performance of
    //     existing well-behaved clients too much.
    // TODO: The right solution is probably to make the compositor ignore the
    //     alpha channel on this.
    case mir_pixel_format_xrgb_8888:
    case mir_pixel_format_argb_8888:
        if (little_endian)
        {
            gl_format = GL_BGRA_EXT;
            gl_type = GL_UNSIGNED_BYTE;
        }
        break;

    // XXX mir_pixel_format_xbgr_8888: This will look wrong unless the client
    //     has filled the X (alpha) bytes with 255.
    // TODO: The right solution is probably to make the compositor ignore the
    //     alpha channel on this.
    case mir_pixel_format_xbgr_8888:
    case mir_pixel_format_abgr_8888:
        if (little_endian)
        {
            gl_format = GL_RGBA;
            gl_type = GL_UNSIGNED_BYTE;
        }
        break;

    /* This future format would also work:
    case mir_pixel_format_rgb_888:
        gl_format = RGB;
        gl_type = GL_UNSIGNED_BYTE;
        break;
    */

    default:
        break;
    }
    
    // XXX Throw on invalid GL formats or silently accept them?
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

void mgm::ShmBuffer::gl_bind_to_texture()
{
    if (gl_format == GL_INVALID_ENUM)
        return;

    glTexImage2D(GL_TEXTURE_2D, 0, gl_format,
                 size_.width.as_int(), size_.height.as_int(),
                 0, gl_format, gl_type,
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

void mgm::ShmBuffer::write(unsigned char const* data, size_t data_size)
{
    if (data_size != stride_.as_uint32_t()*size().height.as_uint32_t())
        BOOST_THROW_EXCEPTION(std::logic_error("Size is not equal to number of pixels in buffer"));
    memcpy(pixels, data, data_size);
}

void mgm::ShmBuffer::read(std::function<void(unsigned char const*)> const& do_with_pixels)
{
    do_with_pixels(static_cast<unsigned char const*>(pixels));
}
