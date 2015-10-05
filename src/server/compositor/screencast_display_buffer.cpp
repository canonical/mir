/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "screencast_display_buffer.h"
#include "mir/graphics/buffer.h"
#include "mir/renderer/gl/texture_source.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mc::ScreencastDisplayBuffer::ScreencastDisplayBuffer(
    geom::Rectangle const& rect,
    mg::Buffer& buffer)
    : rect(rect), buffer(buffer),
      texture_source(
          dynamic_cast<mir::renderer::gl::TextureSource*>(
              buffer.native_buffer_base())),
      old_fbo(), old_viewport()
{
    if (!texture_source)
        BOOST_THROW_EXCEPTION(std::logic_error("Buffer does not support GL rendering"));
    glGetIntegerv(GL_VIEWPORT, old_viewport);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* Set up the color buffer... */
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* and the depth buffer */
    auto const buf_size = buffer.size();
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          buf_size.width.as_uint32_t(),
                          buf_size.height.as_uint32_t());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create FBO for buffer"));
}

mc::ScreencastDisplayBuffer::~ScreencastDisplayBuffer()
{
    release_current();
}

geom::Rectangle mc::ScreencastDisplayBuffer::view_area() const
{
    return rect;
}

void mc::ScreencastDisplayBuffer::make_current()
{
    glBindTexture(GL_TEXTURE_2D, color_tex);
    texture_source->gl_bind_to_texture();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, color_tex, 0);

    auto const buf_size = buffer.size();

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, buf_size.width.as_uint32_t(), buf_size.height.as_uint32_t());
}

void mc::ScreencastDisplayBuffer::release_current()
{
    glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
    glViewport(old_viewport[0], old_viewport[1],
               old_viewport[2], old_viewport[3]);
}

bool mc::ScreencastDisplayBuffer::post_renderables_if_optimizable(mg::RenderableList const&)
{
    return false;
}

void mc::ScreencastDisplayBuffer::swap_buffers()
{
    glFinish();
}

MirOrientation mc::ScreencastDisplayBuffer::orientation() const
{
    return mir_orientation_normal;
}

mg::NativeDisplayBuffer* mc::ScreencastDisplayBuffer::native_display_buffer()
{
    return this;
}
