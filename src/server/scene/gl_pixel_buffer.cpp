/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gl_pixel_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/buffer.h"
#include "mir/renderer/gl/texture_source.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

#include <GLES2/gl2ext.h>

namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{

bool is_big_endian()
{
    uint32_t n = 1;
    return (*reinterpret_cast<char*>(&n) != 1);
}

inline uint32_t abgr_to_argb(uint32_t p)
{
    return ((p << 16) & 0x00ff0000) | /* Move R to new position */
           ((p) & 0x0000ff00) |       /* G remains at same position */
           ((p >> 16) & 0x000000ff) | /* Move B to new position */
           ((p) & 0xff000000);        /* A remains at same position */
}

}

ms::GLPixelBuffer::GLPixelBuffer(std::unique_ptr<graphics::GLContext> gl_context)
    : gl_context{std::move(gl_context)},
      tex{0}, fbo{0}, gl_pixel_format{0}, pixels_need_y_flip{false}
{
    /*
     * TODO: Handle systems that are big-endian, and therefore GL_BGRA doesn't
     * give the 0xAARRGGBB pixel format we need.
     */
    if (is_big_endian())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "GLPixelBuffer doesn't support big endian architectures"));
    }
}

ms::GLPixelBuffer::~GLPixelBuffer() noexcept
{
    /*
     * This may be called from a different thread
     * than the one that called prepare
     */
    if (tex != 0 || fbo != 0)
        gl_context->make_current();

    if (tex != 0)
        glDeleteTextures(1, &tex);
    if (fbo != 0)
        glDeleteFramebuffers(1, &fbo);
}

void ms::GLPixelBuffer::prepare()
{
    gl_context->make_current();

    if (tex == 0)
        glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    glActiveTexture(GL_TEXTURE0);

    if (fbo == 0)
        glGenFramebuffers(1, &fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void ms::GLPixelBuffer::fill_from(graphics::Buffer& buffer)
{
    auto width = buffer.size().width.as_uint32_t();
    auto height = buffer.size().height.as_uint32_t();

    pixels.resize(width * height * 4);

    prepare();

    auto const texture_source =
        dynamic_cast<mir::renderer::gl::TextureSource*>(
            buffer.native_buffer_base());
    if (!texture_source)
        BOOST_THROW_EXCEPTION(std::logic_error("Buffer does not support GL rendering"));
    texture_source->gl_bind_to_texture();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    /* First try to get pixels as BGRA */
    glGetError();
    gl_pixel_format = GL_BGRA_EXT;
    glReadPixels(0, 0, width, height, gl_pixel_format, GL_UNSIGNED_BYTE, pixels.data());

    /* If getting pixels as BGRA failed, fall back to RGBA */
    if (glGetError() != GL_NO_ERROR)
    {
        gl_pixel_format = GL_RGBA;
        glReadPixels(0, 0, width, height, gl_pixel_format, GL_UNSIGNED_BYTE, pixels.data());
    }

    size_ = buffer.size();
    pixels_need_y_flip = true;
}

void const* ms::GLPixelBuffer::as_argb_8888()
{
    if (pixels_need_y_flip)
    {
        auto const stride_val = stride().as_uint32_t();
        auto const height = size_.height.as_uint32_t();

        std::vector<char> tmp(stride_val);

        for (unsigned int i = 0; i < height / 2; i++)
        {
            /* Store line i */
            tmp.assign(&pixels[i * stride_val], &pixels[(i + 1) * stride_val]);

            /* Copy line height - i - 1 to line i */
            copy_and_convert_pixel_line(&pixels[(height - i - 1) * stride_val],
                                        &pixels[i * stride_val]);

            /* Copy stored line (i) to height - i - 1 */
            copy_and_convert_pixel_line(tmp.data(),
                                        &pixels[(height - i - 1) * stride_val]);
        }

        /* Process middle line if there is one */
        if (height % 2 == 1)
        {
            copy_and_convert_pixel_line(&pixels[(height / 2) * stride_val],
                                        &pixels[(height / 2) * stride_val]);
        }

        pixels_need_y_flip = false;
    }

    return pixels.data();
}

geom::Size ms::GLPixelBuffer::size() const
{
    return size_;
}

geom::Stride ms::GLPixelBuffer::stride() const
{
    return geom::Stride{size_.width.as_uint32_t() * sizeof(uint32_t)};
}

void ms::GLPixelBuffer::copy_and_convert_pixel_line(char* src, char* dst)
{
    if (gl_pixel_format == GL_RGBA)
    {
        /* Convert from abgr_8888 to argb_8888 while copying */
        auto pixels_src = reinterpret_cast<uint32_t*>(src);
        auto pixels_dst = reinterpret_cast<uint32_t*>(dst);
        auto const width = size_.width.as_uint32_t();

        for (uint32_t n = 0; n < width; n++)
            pixels_dst[n] = abgr_to_argb(pixels_src[n]);
    }
    else if (src != dst)
    {
        std::copy(src, src + stride().as_uint32_t(), dst);
    }
}
