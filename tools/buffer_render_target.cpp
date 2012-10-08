/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
#include "mir/compositor/buffer.h"
#include "mir/thread/all.h"

#include <GLES2/gl2ext.h>
#include <stdexcept>

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mt = mir::tools;

namespace mir
{
namespace
{
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR_ = 0;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR_ = 0;
PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glEGLImageTargetRenderbufferStorageOES_ = 0;

std::mutex ext_mutex;

void ensure_egl_image_extensions()
{
    std::lock_guard<std::mutex> lock(ext_mutex);

    if (eglCreateImageKHR_ != 0 && eglDestroyImageKHR_ != 0 &&
        glEGLImageTargetRenderbufferStorageOES_ != 0)
    {
        return;
    }

    std::string ext_string;
    const char* exts = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    if (exts)
        ext_string = exts;

    /* Mesa in the framebuffer doesn't advertise EGL_KHR_image_pixmap properly */
    //if (ext_string.find("EGL_KHR_image_pixmap") != std::string::npos)
    {
        eglCreateImageKHR_ =
            reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        eglDestroyImageKHR_ =
            reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    }

    if (!eglCreateImageKHR_ || !eglDestroyImageKHR_)
        throw std::runtime_error("EGL implementation doesn't support EGLImage");

    exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (exts)
        ext_string = exts;
    else
        ext_string.clear();

    if (ext_string.find("GL_OES_EGL_image") != std::string::npos)
    {
        glEGLImageTargetRenderbufferStorageOES_ =
            reinterpret_cast<PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC>(
                eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES"));
    }

    if (!glEGLImageTargetRenderbufferStorageOES_)
        throw std::runtime_error("GLES2 implementation doesn't support updating a renderbuffer from an EGLImage");
}

}
}

mt::BufferRenderTarget::BufferRenderTarget(const mc::Buffer& buffer,
                                           EGLClientBuffer client_buffer)
    : buffer(buffer)
{
    resources.setup(buffer, client_buffer);
}

void mt::BufferRenderTarget::make_current()
{
    geom::Size buf_size = buffer.size();

    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);
    glViewport(0, 0, buf_size.width.as_uint32_t(), buf_size.height.as_uint32_t());
}

mt::BufferRenderTarget::Resources::~Resources()
{
    if (egl_image != EGL_NO_IMAGE_KHR)
        (*eglDestroyImageKHR_)(eglGetCurrentDisplay(), egl_image);
    if (color_rbo != 0)
        glDeleteRenderbuffers(1, &color_rbo);
    if (depth_rbo != 0)
        glDeleteRenderbuffers(1, &depth_rbo);
    if (fbo != 0)
        glDeleteFramebuffers(1, &fbo);
}

void mt::BufferRenderTarget::Resources::setup(const mc::Buffer& buffer,
                                              EGLClientBuffer client_buffer)
{
    geom::Size buf_size = buffer.size();
    const EGLint image_attrs[] =
    {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE
    };

    if (egl_image == EGL_NO_IMAGE_KHR)
    {
        ensure_egl_image_extensions();

        egl_image = (*eglCreateImageKHR_)(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                                          EGL_NATIVE_PIXMAP_KHR, client_buffer,
                                          image_attrs);
        if (egl_image == EGL_NO_IMAGE_KHR)
            throw std::runtime_error("Failed to create EGLImage from DRM buffer");
    }

    if (fbo == 0)
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        /* Set up color buffer... */
        glGenRenderbuffers(1, &color_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, color_rbo);
        (*glEGLImageTargetRenderbufferStorageOES_)(GL_RENDERBUFFER, egl_image);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, color_rbo);

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
