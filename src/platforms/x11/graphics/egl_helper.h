/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_X11_EGL_HELPER_H_
#define MIR_GRAPHICS_X11_EGL_HELPER_H_

#include <memory>
#include <functional>

#include <xcb/xcb.h>
#include <EGL/egl.h>

#include "mir/graphics/platform.h"

typedef struct _XDisplay Display;

namespace mir
{
namespace graphics
{
class GLConfig;

namespace X
{

namespace helpers
{
class Framebuffer : public GenericEGLDisplayProvider::EGLFramebuffer
{
public:
    /**
     * Handle for an EGL output surface
     *
     * \note    This takes ownership of \param ctx and \param surf; when the
     *          final handle generated from this Framebuffer is released,
     *          the EGL resources \param ctx and \param surff will be freed.
     */
    Framebuffer(EGLDisplay dpy, EGLContext ctx, EGLSurface surf, geometry::Size size);

    auto size() const -> geometry::Size override;

    void make_current() override;
    void release_current() override;
    auto clone_handle() -> std::unique_ptr<GenericEGLDisplayProvider::EGLFramebuffer> override;

    void swap_buffers();
private:
    class EGLState;
    Framebuffer(std::shared_ptr<EGLState const> surf, geometry::Size size);

    std::shared_ptr<EGLState const> const state;
    geometry::Size const size_;
};

class EGLHelper
{
public:
    EGLHelper(const EGLHelper&) = delete;
    EGLHelper& operator=(const EGLHelper&) = delete;

    explicit EGLHelper(::Display* const x_dpy);
    EGLHelper(GLConfig const& gl_config, ::Display* const x_dpy);
    EGLHelper(GLConfig const& gl_config, ::Display* const x_dpy, EGLContext shared_context);
    // Passing an XLib Display and an XCB window is, in fact, not a mistake
    EGLHelper(GLConfig const& gl_config, ::Display* const x_dpy, xcb_window_t win, EGLContext shared_context);
    ~EGLHelper() noexcept;

    auto framebuffer_for_window(
        GLConfig const& conf,
        xcb_connection_t* xcb_conn,
        xcb_window_t win,
        EGLContext shared_context)
        -> std::unique_ptr<Framebuffer>;

    EGLDisplay display() const { return egl_display; }
    EGLConfig config() const { return egl_config; }
    EGLSurface surface() const { return egl_surface; }
    EGLContext context() const { return egl_context; }

    void report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)>) const;
private:
    EGLHelper(int stencil_bits, int depth_bits);
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
