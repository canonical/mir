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

#include "mir/graphics/gl_context.h"
#include "mir/graphics/gl_pixel_buffer.h"
#include "mir/compositor/buffer.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

#include <GLES2/gl2ext.h>

namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{

bool is_big_endian()
{
    uint32_t n = 1;
    return (*reinterpret_cast<char*>(&n) != 1);
}

}

mg::GLPixelBuffer::GLPixelBuffer(std::unique_ptr<GLContext> gl_context)
    : gl_context{std::move(gl_context)},
      tex{0}, fbo{0}, pixels_need_y_flip{false}
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

mg::GLPixelBuffer::~GLPixelBuffer() noexcept
{
    if (tex != 0)
        glDeleteTextures(1, &tex);
    if (fbo != 0)
        glDeleteFramebuffers(1, &fbo);
}

void mg::GLPixelBuffer::prepare()
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

void mg::GLPixelBuffer::fill_from(compositor::Buffer& buffer)
{
    auto width = buffer.size().width.as_uint32_t();
    auto height = buffer.size().height.as_uint32_t();

    pixels.resize(width * height * 4);

    prepare();

    buffer.bind_to_texture();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    /* TODO: Handle systems that don't support GL_BGRA_EXT for glReadPixels() */
    glGetError();
    glReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels.data());
    if (glGetError() == GL_INVALID_ENUM)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "GLPixelBuffer couldn't read pixels in BGRA format"));
    }

    size_ = buffer.size();
    pixels_need_y_flip = true;
}

void const* mg::GLPixelBuffer::as_argb_8888()
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
            std::copy(&pixels[(height - i - 1) * stride_val],
                      &pixels[(height - i) * stride_val],
                      &pixels[i * stride_val]);

            /* Copy stored line (i) to height - i - 1 */
            std::copy(tmp.begin(), tmp.end(),
                      &pixels[(height - i - 1) * stride_val]);
        }

        pixels_need_y_flip = false;
    }

    return pixels.data();
}

geom::Size mg::GLPixelBuffer::size() const
{
    return size_;
}

geom::Stride mg::GLPixelBuffer::stride() const
{
    return geom::Stride{size_.width.as_uint32_t() * 4};
}
