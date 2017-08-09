/*
 * Copyright © 2013 Canonical Ltd.
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
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_EGL_EXTENSIONS_H_
#define MIR_GRAPHICS_EGL_EXTENSIONS_H_

#include <experimental/optional>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

namespace mir
{
namespace graphics
{
struct EGLExtensions
{
    EGLExtensions();
    PFNEGLCREATEIMAGEKHRPROC const eglCreateImageKHR;
    PFNEGLDESTROYIMAGEKHRPROC const eglDestroyImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC const glEGLImageTargetTexture2DOES;

    struct WaylandExtensions
    {
        WaylandExtensions();

        PFNEGLBINDWAYLANDDISPLAYWL const eglBindWaylandDisplayWL;
        PFNEGLQUERYWAYLANDBUFFERWL const eglQueryWaylandBufferWL;
    };
    std::experimental::optional<WaylandExtensions> const wayland;
};

}
}

#endif /* MIR_GRAPHICS_EGL_EXTENSIONS_H_ */
