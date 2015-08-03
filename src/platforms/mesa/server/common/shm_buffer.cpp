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
#include <endian.h>

namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;

namespace {

bool get_gl_pixel_format(MirPixelFormat mir_format,
                         GLenum& gl_format, GLenum& gl_type)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    GLenum const argb = GL_BGRA_EXT;
    GLenum const abgr = GL_RGBA;
#elif __BYTE_ORDER == __BIG_ENDIAN
    // TODO: Big endian support
    GLenum const argb = GL_INVALID_ENUM;
    GLenum const abgr = GL_INVALID_ENUM;
    //GLenum const rgba = GL_RGBA;
    //GLenum const bgra = GL_BGRA_EXT;
#endif

    static const struct
    {
        MirPixelFormat mir_format;
        GLenum gl_format, gl_type;
    } mapping[mir_pixel_formats] =
    {
        {mir_pixel_format_invalid,   GL_INVALID_ENUM, GL_INVALID_ENUM},
        {mir_pixel_format_abgr_8888, abgr,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_xbgr_8888, abgr,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_argb_8888, argb,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_xrgb_8888, argb,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_bgr_888,   GL_INVALID_ENUM, GL_INVALID_ENUM},
        {mir_pixel_format_rgb_888,   GL_RGB,          GL_UNSIGNED_BYTE},
        {mir_pixel_format_rgb_565,   GL_RGB,          GL_UNSIGNED_SHORT_5_6_5},
        {mir_pixel_format_rgba_5551, GL_RGBA,         GL_UNSIGNED_SHORT_5_5_5_1},
        {mir_pixel_format_rgba_4444, GL_RGBA,         GL_UNSIGNED_SHORT_4_4_4_4},
    };

    if (mir_format > mir_pixel_format_invalid &&
        mir_format < mir_pixel_formats &&
        mapping[mir_format].mir_format == mir_format) // just a sanity check
    {
        gl_format = mapping[mir_format].gl_format;
        gl_type = mapping[mir_format].gl_type;
    }
    else
    {
        gl_format = GL_INVALID_ENUM;
        gl_type = GL_INVALID_ENUM;
    }

    return gl_format != GL_INVALID_ENUM && gl_type != GL_INVALID_ENUM;
}

} // anonymous namespace

bool mgm::ShmBuffer::supports(MirPixelFormat mir_format)
{
    GLenum gl_format, gl_type;
    return get_gl_pixel_format(mir_format, gl_format, gl_type);
}

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

void mgm::ShmBuffer::gl_bind_to_texture()
{
    GLenum format, type;

    if (get_gl_pixel_format(pixel_format_, format, type))
    {
        /*
         * All existing Mir logic assumes that strides are whole multiples of
         * pixels. And OpenGL defaults to expecting strides are multiples of
         * 4 bytes. These assumptions used to be compatible when we only had
         * 4-byte pixels but now we support 2/3-byte pixels we need to be more
         * careful...
         */
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, format,
                     size_.width.as_int(), size_.height.as_int(),
                     0, format, type, pixels);
    }
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
