/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "buffer_render_target.h"
#include "mir/graphics/buffer.h"
#include "mir/renderer/gl/texture_source.h"

#include <GLES2/gl2ext.h>
#include <stdexcept>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mt = mir::tools;

mt::BufferRenderTarget::BufferRenderTarget(mg::Buffer& buffer)
    : buffer(buffer), old_fbo(), old_viewport()
{
    /*
     * With the new lazy buffer allocation method, we may be executing inside
     * the compositor's GL context. So be careful to save and restore what
     * we change...
     */
    glGetIntegerv(GL_VIEWPORT, old_viewport);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
    resources.setup(buffer);
}

mt::BufferRenderTarget::~BufferRenderTarget()
{
    glFinish();
    glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
    glViewport(old_viewport[0], old_viewport[1],
               old_viewport[2], old_viewport[3]);
}

void mt::BufferRenderTarget::make_current()
{
    geom::Size buf_size = buffer.size();

    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);
    glViewport(0, 0, buf_size.width.as_uint32_t(), buf_size.height.as_uint32_t());
}

mt::BufferRenderTarget::Resources::~Resources()
{
    if (color_tex != 0)
        glDeleteTextures(1, &color_tex);
    if (depth_rbo != 0)
        glDeleteRenderbuffers(1, &depth_rbo);
    if (fbo != 0)
        glDeleteFramebuffers(1, &fbo);
}

void mt::BufferRenderTarget::Resources::setup(mg::Buffer& buffer)
{
    geom::Size buf_size = buffer.size();

    if (fbo == 0)
    {
        auto const texture_source =
            dynamic_cast<mir::renderer::gl::TextureSource*>(buffer.native_buffer_base());
        if (!texture_source)
            throw std::logic_error("Buffer does not support GL rendering");

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        /* Set up color buffer... */
        glGenTextures(1, &color_tex);
        glBindTexture(GL_TEXTURE_2D, color_tex);
        texture_source->gl_bind_to_texture();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, color_tex, 0);

        /* and depth buffer */
        glGenRenderbuffers(1, &depth_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                              buf_size.width.as_uint32_t(), buf_size.height.as_uint32_t());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Failed to create FBO for GBM buffer");
    }
}
