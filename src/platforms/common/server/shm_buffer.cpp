/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#include "mir/graphics/gl_format.h"
#include "mir/shm_file.h"
#include "shm_buffer.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "egl_context_delegate.h"

#define MIR_LOG_COMPONENT "gfx-common"
#include "mir/log.h"

#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

#include <boost/throw_exception.hpp>

#include <stdexcept>

#include <string.h>
#include <endian.h>

namespace mg=mir::graphics;
namespace mgc = mir::graphics::common;
namespace geom = mir::geometry;

bool mg::get_gl_pixel_format(MirPixelFormat mir_format,
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

bool mgc::ShmBuffer::supports(MirPixelFormat mir_format)
{
    GLenum gl_format, gl_type;
    return mg::get_gl_pixel_format(mir_format, gl_format, gl_type);
}

mgc::ShmBuffer::ShmBuffer(
    geom::Size const& size,
    MirPixelFormat const& format,
    std::shared_ptr<EGLContextDelegate> egl_delegate)
    : size_{size},
      pixel_format_{format},
      egl_delegate{std::move(egl_delegate)}
{
}

mgc::MemoryBackedShmBuffer::MemoryBackedShmBuffer(
    geom::Size const& size,
    MirPixelFormat const& pixel_format,
    std::shared_ptr<EGLContextDelegate> egl_delegate)
    : ShmBuffer(size, pixel_format, std::move(egl_delegate)),
      stride_{MIR_BYTES_PER_PIXEL(pixel_format) * size.width.as_uint32_t()},
      pixels{new unsigned char[stride_.as_int() * size.height.as_int()]}
{
}

mgc::ShmBuffer::~ShmBuffer() noexcept
{
    if (tex_id != 0)
    {
        egl_delegate->defer_to_egl_context(
            [id = tex_id]()
            {
                glDeleteTextures(1, &id);
            });
    }
}

geom::Size mgc::ShmBuffer::size() const
{
    return size_;
}

geom::Stride mgc::MemoryBackedShmBuffer::stride() const
{
    return stride_;
}

MirPixelFormat mgc::ShmBuffer::pixel_format() const
{
    return pixel_format_;
}

void mgc::ShmBuffer::upload_to_texture(void const* pixels, geom::Stride const& stride)
{
    GLenum format, type;

    if (mg::get_gl_pixel_format(pixel_format_, format, type))
    {
        auto const stride_in_px =
            stride.as_int() / MIR_BYTES_PER_PIXEL(pixel_format());
        /*
         * We assume (as does Weston, AFAICT) that stride is
         * a multiple of whole pixels, but it need not be.
         *
         * TODO: Handle non-pixel-multiple strides.
         * This should be possible by calculating GL_UNPACK_ALIGNMENT
         * to match the size of the partial-pixel-stride().
         */

        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride_in_px);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            size().width.as_int(), size().height.as_int(),
            0,
            format,
            type,
            pixels);

        // Be nice to other users of the GL context by reverting our changes to shared state
        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);     // 0 is default, meaning “use width”
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);          // 4 is default; word alignment.
    }
    else
    {
        mir::log_error(
            "Buffer %i has non-GL-compatible pixel format %i; rendering will be incomplete",
            id().as_value(),
            pixel_format());
    }
}

void mgc::MemoryBackedShmBuffer::write(unsigned char const* data, size_t data_size)
{
    if (data_size != stride_.as_uint32_t()*size().height.as_uint32_t())
        BOOST_THROW_EXCEPTION(std::logic_error("Size is not equal to number of pixels in buffer"));
    memcpy(pixels.get(), data, data_size);
}

void mgc::MemoryBackedShmBuffer::read(std::function<void(unsigned char const*)> const& do_with_pixels)
{
    do_with_pixels(static_cast<unsigned char const*>(pixels.get()));
}

mg::NativeBufferBase* mgc::ShmBuffer::native_buffer_base()
{
    return this;
}

void mgc::ShmBuffer::bind()
{
    bool const needs_initialisation = tex_id == 0;
    if (needs_initialisation)
    {
        glGenTextures(1, &tex_id);
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    if (needs_initialisation)
    {
        // The ShmBuffer *should* be immutable, so we can just upload once.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

void mgc::MemoryBackedShmBuffer::bind()
{
    mgc::ShmBuffer::bind();
    upload_to_texture(pixels.get(), stride_);
}

auto mgc::MemoryBackedShmBuffer::native_buffer_handle() const -> std::shared_ptr<mg::NativeBuffer>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"MemoryBackedShmBuffer does not support mirclient APIs"}));
}

mg::gl::Program const& mgc::ShmBuffer::shader(mg::gl::ProgramFactory& cache) const
{
    static auto const program = cache.compile_fragment_shader(
        "",
        "uniform sampler2D tex;\n"
        "vec4 sample_to_rgba(in vec2 texcoord)\n"
        "{\n"
        "    return texture2D(tex, texcoord);\n"
        "}\n");

    return *program;
}

auto mgc::ShmBuffer::layout() const -> Layout
{
    return Layout::GL;
}

void mgc::ShmBuffer::add_syncpoint()
{
}

