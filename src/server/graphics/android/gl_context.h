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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_
#define MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_

namespace mir
{
namespace graphics
{
namespace android
{

//handy functions
EGLSurface create_dummy_pbuffer_surface(EGLDisplay, EGLConfig, EGLContext);
EGLSurface create_window_surface(EGLDisplay, EGLConfig, EGLContext, EGLNativeWindowType);

class GLContext : public graphics::GLContext
{
public:
    //For creating a gl context
    EGLHelper(geom::PixelFormat display_format);

    //For creating a gl context shared with another EGLHelper
    EGLHelper(EGLHelper copied_helper,
              std::function<EGLSurface(EGLDisplay, EGLConfig, EGLContext)> const& create_egl_surface);

    ~EGLHelper();

    make_current();
    swap_buffers();
    release_current();

private:
    EGLDisplay const egl_display;
    bool const own_display;
    EGLConfig const egl_config;

    EGLContextStore const egl_context_shared;
    EGLSurfaceStore const egl_surface;
}

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_GL_CONTEXT_H_ */
