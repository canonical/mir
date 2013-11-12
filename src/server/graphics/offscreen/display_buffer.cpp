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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_buffer.h"
#include "gl_extensions_base.h"

#include <boost/throw_exception.hpp>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace mg = mir::graphics;
namespace mgo = mg::offscreen;
namespace geom = mir::geometry;

namespace
{

EGLint const default_egl_context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

class GLExtensions : public mgo::GLExtensionsBase
{
public:
    GLExtensions() :
        mgo::GLExtensionsBase{
            reinterpret_cast<char const*>(glGetString(GL_EXTENSIONS))}
    {
    }
};

}

mgo::detail::GLFramebufferObject::GLFramebufferObject(geom::Size const& size)
    : size{size}, color_renderbuffer{0}, depth_renderbuffer{0}, fbo{0}
{
    GLExtensions const extensions;

    GLenum gl_color_format{GL_RGBA4};
    GLenum const gl_depth_format{GL_DEPTH_COMPONENT16};

    if (extensions.support("GL_ARM_rgba8") ||
        extensions.support("GL_OES_rgb8_rgba8"))
    {
        gl_color_format = GL_RGBA8_OES;
    }

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
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, color_renderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_renderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

mgo::detail::GLFramebufferObject::~GLFramebufferObject()
{
    if (fbo)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (color_renderbuffer)
    {
        glDeleteRenderbuffers(1, &color_renderbuffer);
        color_renderbuffer = 0;
    }
    if (depth_renderbuffer)
    {
        glDeleteRenderbuffers(1, &depth_renderbuffer);
        depth_renderbuffer = 0;
    }
}

void mgo::detail::GLFramebufferObject::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, size.width.as_int(), size.height.as_int());
}

void mgo::detail::GLFramebufferObject::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    fbo.bind();
}

void mgo::DisplayBuffer::release_current()
{
    fbo.unbind();
    egl_context.release_current();
}

void mgo::DisplayBuffer::post_update()
{
    glFinish();
}

bool mgo::DisplayBuffer::can_bypass() const
{
    return false;
}
