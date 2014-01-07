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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_
#define MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_

#include "mir/graphics/gl_context.h"
#include "mir/graphics/egl_resources.h"
#include "mir_toolkit/common.h"
#include <functional>

namespace mir
{
namespace graphics
{
class DisplayReport;
namespace android
{

//handy functions
EGLSurface create_dummy_pbuffer_surface(EGLDisplay, EGLConfig);
EGLSurface create_window_surface(EGLDisplay, EGLConfig, EGLNativeWindowType);

class GLContext : public graphics::GLContext
{
public:
    //For creating a gl context
    GLContext(MirPixelFormat display_format, DisplayReport& report);

    //For creating a gl context shared with another GLContext
    GLContext(GLContext const& shared_gl_context,
              std::function<EGLSurface(EGLDisplay, EGLConfig)> const& create_egl_surface);

    ~GLContext();

    void make_current() const override;
    void swap_buffers();
    void release_current() const override;

    /* TODO: (kdub) remove these two functions once HWC1.0 construction is sorted out. */
    EGLDisplay display()
    {
        return egl_display;
    }
    EGLSurface surface()
    {
        return egl_surface;
    }

private:
    EGLDisplay const egl_display;
    bool const own_display;
    EGLConfig const egl_config;

    EGLContextStore const egl_context;
    EGLSurfaceStore const egl_surface;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_ */
