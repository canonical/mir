/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "display_buffer.h"
#include "mir/graphics/gl_extensions_base.h"
#include "mir/raii.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

namespace mg = mir::graphics;
namespace mgo = mg::offscreen;
namespace geom = mir::geometry;

namespace
{

class GLExtensions : public mg::GLExtensionsBase
{
public:
    GLExtensions() :
        mg::GLExtensionsBase{
            reinterpret_cast<char const*>(glGetString(GL_EXTENSIONS))}
    {
    }
};

}

mgo::detail::GLFramebufferObject::GLFramebufferObject(geom::Size const& size)
    : size{size}, color_renderbuffer{0}, depth_renderbuffer{0}, fbo{0}
{
    /* Save previous FBO state */
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
    glGetIntegerv(GL_VIEWPORT, old_viewport);

    GLExtensions const extensions;

    GLenum gl_color_format{GL_RGBA4};
    GLenum const gl_depth_format{GL_DEPTH_COMPONENT16};

#ifdef GL_RGBA8_OES
    if (extensions.support("GL_ARM_rgba8") ||
        extensions.support("GL_OES_rgb8_rgba8"))
    {
        gl_color_format = GL_RGBA8_OES;
    }
#endif

    /* Create a renderbuffer for the color attachment */
    glGenRenderbuffers(1, &color_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, gl_color_format,
                          size.width.as_int(), size.height.as_int());

    /* Create a renderbuffer for the depth attachment */
    glGenRenderbuffers(1, &depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, gl_depth_format,
                          size.width.as_int(), size.height.as_int());

    /* Create a FBO and set it up */
    glGenFramebuffers(1, &fbo);

    auto const fbo_raii = mir::raii::paired_calls(
        [this] { glBindFramebuffer(GL_FRAMEBUFFER, fbo); },
        [this] { glBindFramebuffer(GL_FRAMEBUFFER, old_fbo); });

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, color_renderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_renderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to set up FBO"));
}

mgo::detail::GLFramebufferObject::~GLFramebufferObject()
{
    if (fbo)
        glDeleteFramebuffers(1, &fbo);
    if (color_renderbuffer)
        glDeleteRenderbuffers(1, &color_renderbuffer);
    if (depth_renderbuffer)
        glDeleteRenderbuffers(1, &depth_renderbuffer);
}

void mgo::detail::GLFramebufferObject::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, size.width.as_int(), size.height.as_int());
}

void mgo::detail::GLFramebufferObject::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
    glViewport(old_viewport[0], old_viewport[1],
               old_viewport[2], old_viewport[3]);
}

mgo::DisplayBuffer::DisplayBuffer(SurfacelessEGLContext egl_context,
                                  geom::Rectangle const& area)
    : egl_context{std::move(egl_context)},
      fbo{area.size},
      area(area)
{
}

geom::Rectangle mgo::DisplayBuffer::view_area() const
{
    return area;
}

void mgo::DisplayBuffer::make_current()
{
    egl_context.make_current();
}

void mgo::DisplayBuffer::bind()
{
    fbo.bind();
}

void mgo::DisplayBuffer::release_current()
{
    fbo.unbind();
    egl_context.release_current();
}

void mgo::DisplayBuffer::swap_buffers()
{
    glFinish();
}

bool mgo::DisplayBuffer::overlay(RenderableList const&)
{
    return false;
}

glm::mat2 mgo::DisplayBuffer::transformation() const
{
    return glm::mat2(1);
}

mg::NativeDisplayBuffer* mgo::DisplayBuffer::native_display_buffer()
{
    return this;
}
