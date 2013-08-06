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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_EGL_RESOURCES_H_
#define MIR_GRAPHICS_EGL_RESOURCES_H_

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{

class EGLContextStore
{
public:
    EGLContextStore(EGLDisplay egl_display, EGLContext egl_context);
    ~EGLContextStore() noexcept;

    operator EGLContext() const;

private:
    EGLContextStore(EGLContextStore const&) = delete;
    EGLContextStore& operator=(EGLContextStore const&) = delete;

    EGLDisplay const egl_display_;
    EGLContext const egl_context_;
};

class EGLSurfaceStore
{
public:
    EGLSurfaceStore(EGLDisplay egl_display, EGLSurface egl_surface);

    ~EGLSurfaceStore() noexcept;

    operator EGLSurface() const;

private:
    EGLSurfaceStore(EGLSurfaceStore const&) = delete;
    EGLSurfaceStore& operator=(EGLSurfaceStore const&) = delete;

    EGLDisplay const egl_display_;
    EGLSurface const egl_surface_;
};

}
}

#endif /* MIR_GRAPHICS_EGL_RESOURCES_H_ */
