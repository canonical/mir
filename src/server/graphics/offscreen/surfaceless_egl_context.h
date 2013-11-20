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

#ifndef MIR_GRAPHICS_OFFSCREEN_SURFACELESS_EGL_CONTEXT_H_
#define MIR_GRAPHICS_OFFSCREEN_SURFACELESS_EGL_CONTEXT_H_

#include "mir/graphics/egl_resources.h"

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace offscreen
{

class SurfacelessEGLContext
{
public:
    SurfacelessEGLContext(EGLDisplay egl_display, EGLContext shared_context);
    SurfacelessEGLContext(SurfacelessEGLContext&&) = default;

    void make_current() const;
    void release_current() const;

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
}

#endif /* MIR_GRAPHICS_OFFSCREEN_SURFACELESS_EGL_SURFACE_H_ */
