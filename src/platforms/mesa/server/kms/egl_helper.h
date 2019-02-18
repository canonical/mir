/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_EGL_HELPER_H_
#define MIR_GRAPHICS_MESA_EGL_HELPER_H_

#include "display_helpers.h"
#include "mir/graphics/egl_extensions.h"
#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
class GLConfig;
namespace mesa
{
namespace helpers
{
class EGLHelper
{
public:
    EGLHelper(GLConfig const& gl_config);
    EGLHelper(
        GLConfig const& gl_config,
        GBMHelper const& gbm,
        gbm_surface* surface,
        EGLContext shared_context);
    ~EGLHelper() noexcept;
    EGLHelper(EGLHelper&& from);

    EGLHelper(const EGLHelper&) = delete;
    EGLHelper& operator=(const EGLHelper&) = delete;

    void setup(GBMHelper const& gbm);
    void setup(GBMHelper const& gbm, EGLContext shared_context);
    void setup(GBMHelper const& gbm, gbm_surface* surface_gbm,
               EGLContext shared_context);

    bool swap_buffers();
    bool make_current() const;
    bool release_current() const;

    EGLContext context() const { return egl_context; }

    void report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)>);
private:
    void setup_internal(GBMHelper const& gbm, bool initialize, EGLint gbm_format);

    EGLint const depth_buffer_bits;
    EGLint const stencil_buffer_bits;
    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;
    EGLSurface egl_surface;
    bool should_terminate_egl;
    EGLExtensions::PlatformBaseEXT platform_base;
};
}
}
}
}

#endif /* MIR_GRAPHICS_MESA_EGL_HELPER_H_ */
