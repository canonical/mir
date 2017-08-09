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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/egl_extensions.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

#define MIR_LOG_COMPONENT "EGL extensions"
#include "mir/log.h"

namespace mg=mir::graphics;

namespace
{
std::experimental::optional<mg::EGLExtensions::WaylandExtensions> maybe_wayland_ext()
{
    try
    {
        return mg::EGLExtensions::WaylandExtensions{};
    }
    catch (std::runtime_error const&)
    {
        return {};
    }
}
}

mg::EGLExtensions::EGLExtensions() :
    eglCreateImageKHR{
        reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"))},
    eglDestroyImageKHR{
        reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"))},

    /*
     * TODO: Find a non-ES GL equivalent for glEGLImageTargetTexture2DOES
     * It's the LAST remaining ES-specific function. Although Mesa lets you use
     * it in full GL, it theoretically should not work. Mesa just lets you
     * mix ES and GL code. But other drivers won't be so lenient.
     */
    glEGLImageTargetTexture2DOES{
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"))},
    wayland{maybe_wayland_ext()}
{
    if (!eglCreateImageKHR || !eglDestroyImageKHR)
        BOOST_THROW_EXCEPTION(std::runtime_error("EGL implementation doesn't support EGLImage"));

    if (!glEGLImageTargetTexture2DOES)
        BOOST_THROW_EXCEPTION(std::runtime_error("GLES2 implementation doesn't support updating a texture from an EGLImage"));
}

mg::EGLExtensions::WaylandExtensions::WaylandExtensions() :
    eglBindWaylandDisplayWL{
        reinterpret_cast<PFNEGLBINDWAYLANDDISPLAYWL>(eglGetProcAddress("eglBindWaylandDisplayWL"))
    },
    eglQueryWaylandBufferWL{
        reinterpret_cast<PFNEGLQUERYWAYLANDBUFFERWL>(eglGetProcAddress("eglQueryWaylandBufferWL"))
    }
{
    if (!eglBindWaylandDisplayWL || !eglQueryWaylandBufferWL)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("EGL implementation doesn't support EGL_WL_bind_wayland_display"));
    }
}