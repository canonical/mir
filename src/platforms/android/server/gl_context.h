/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_
#define MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_

#include "mir/graphics/gl_context.h"
#include "mir/graphics/egl_resources.h"
#include "swapping_gl_context.h"
#include "mir_toolkit/common.h"
#include <functional>

namespace mir
{
namespace graphics
{
class DisplayReport;
class GLConfig;
namespace android
{

class FramebufferBundle;

//helper base class that doesn't have an egl surface.
class GLContext : public graphics::GLContext
{
public:
    ~GLContext();

protected:
    GLContext(MirPixelFormat display_format,
              GLConfig const& gl_config,
              DisplayReport& report);

    GLContext(GLContext const& shared_gl_context);

    void release_current() const override;
    using graphics::GLContext::make_current;
    void make_current(EGLSurface) const;

    EGLDisplay const egl_display;
    EGLConfig const egl_config;
    EGLContextStore const egl_context;

private:
    bool const own_display;
};

class PbufferGLContext : public GLContext
{
public:
    PbufferGLContext(MirPixelFormat display_format,
              GLConfig const& gl_config,
              DisplayReport& report);

    PbufferGLContext(PbufferGLContext const& shared_gl_context);

    void make_current() const override;
    void release_current() const override;
private:
    EGLSurfaceStore const egl_surface;
};


class FramebufferGLContext : public GLContext,
                             public SwappingGLContext
{
public:
    FramebufferGLContext(GLContext const& shared_gl_context,
              std::shared_ptr<FramebufferBundle> const& fb_bundle,
              std::shared_ptr<ANativeWindow> const& native_window);

    void make_current() const override;
    void release_current() const override;
    void swap_buffers() const override;
    std::shared_ptr<Buffer> last_rendered_buffer() const override;

private:
    std::shared_ptr<FramebufferBundle> const fb_bundle;
    EGLSurfaceStore const egl_surface;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_ */
