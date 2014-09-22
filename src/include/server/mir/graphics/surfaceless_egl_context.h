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

#ifndef MIR_GRAPHICS_SURFACELESS_EGL_CONTEXT_H_
#define MIR_GRAPHICS_SURFACELESS_EGL_CONTEXT_H_

#include "mir/graphics/egl_resources.h"
#include "mir/graphics/gl_context.h"

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{

class SurfacelessEGLContext : public GLContext
{
public:
    SurfacelessEGLContext(EGLDisplay egl_display, EGLContext shared_context);
    SurfacelessEGLContext(EGLDisplay egl_display, EGLint const* attribs, EGLContext shared_context);
    /* We have to explicitly define this, as GLContext has a deleted copy constructor */
    SurfacelessEGLContext(SurfacelessEGLContext&& move);
    virtual ~SurfacelessEGLContext() noexcept;

    void make_current() const override;
    void release_current() const override;

    operator EGLContext() const;

private:
    SurfacelessEGLContext(SurfacelessEGLContext const&) = delete;
    SurfacelessEGLContext& operator=(SurfacelessEGLContext const&) = delete;

    EGLDisplay egl_display;
    bool surfaceless;
    EGLConfig egl_config;
    graphics::EGLSurfaceStore egl_surface;
    graphics::EGLContextStore egl_context;
};

}
}

#endif /* MIR_GRAPHICS_SURFACELESS_EGL_SURFACE_H_ */
