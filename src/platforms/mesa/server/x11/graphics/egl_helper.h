/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_GRAPHICS_X11_EGL_HELPER_H_
#define MIR_GRAPHICS_X11_EGL_HELPER_H_

#include <memory>
#include <functional>

#include <X11/Xlib.h>
#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
class GLConfig;

namespace X
{

namespace helpers
{

class EGLHelper
{
public:
    EGLHelper(GLConfig const& gl_config);
    ~EGLHelper() noexcept;

    EGLHelper(const EGLHelper&) = delete;
    EGLHelper& operator=(const EGLHelper&) = delete;

    void setup(::Display* const x_dpy);
    void setup(::Display* const x_dpy, EGLContext shared_context);
    void setup(::Display* const x_dpy, Window win,
               EGLContext shared_context);

    bool swap_buffers();
    bool make_current() const;
    bool release_current() const;

    EGLContext context() const { return egl_context; }
    EGLDisplay display() const { return egl_display; }
    EGLConfig config() const { return egl_config; }
    EGLSurface surface() const { return egl_surface; }

    void report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)>) const;
private:
    void setup_internal(::Display* const x_dpy, bool initialize);

    EGLint const depth_buffer_bits;
    EGLint const stencil_buffer_bits;
    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;
    EGLSurface egl_surface;
    bool should_terminate_egl;
};

}
}
}
}
#endif /* MIR_GRAPHICS_X11_EGL_HELPER_H_ */
