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
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_EGL_EXTENSIONS_H_
#define MIR_GRAPHICS_EGL_EXTENSIONS_H_

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

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
};
}
}

#endif /* MIR_GRAPHICS_EGL_EXTENSIONS_H_ */
