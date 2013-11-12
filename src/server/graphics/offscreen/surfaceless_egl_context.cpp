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

#include "surfaceless_egl_context.h"
#include "gl_extensions_base.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mgo = mir::graphics::offscreen;

namespace
{

class EGLExtensions : public mgo::GLExtensionsBase
{
public:
    EGLExtensions(EGLDisplay egl_display) :
        mgo::GLExtensionsBase{
            reinterpret_cast<char const*>(eglQueryString(egl_display, EGL_EXTENSIONS))}
    {
    }
};

bool supports_surfaceless_context(EGLDisplay egl_display)
{
    EGLExtensions const extensions{egl_display};

    return extensions.support("EGL_KHR_surfaceless_context");
}

EGLConfig choose_config(EGLDisplay egl_display, bool surfaceless)
{
    EGLExtensions const extensions{egl_display};

    EGLint const surface_type = surfaceless ? EGL_DONT_CARE : EGL_PBUFFER_BIT;

    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, surface_type,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig egl_config{0};
    int num_egl_configs{0};

    if (eglChooseConfig(egl_display, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to choose ARGB EGL config"));
    }

    return egl_config;
}

EGLSurface create_surface(EGLDisplay egl_display, EGLConfig egl_config)
{
    static EGLint const dummy_pbuffer_attribs[] =
    {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };

    return eglCreatePbufferSurface(egl_display, egl_config, dummy_pbuffer_attribs);
}

EGLint const default_egl_context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

}

mgo::SurfacelessEGLContext::SurfacelessEGLContext(
    EGLDisplay egl_display,
    EGLContext shared_context)
    : egl_display{egl_display},
      surfaceless{supports_surfaceless_context(egl_display)},
      egl_config{choose_config(egl_display, surfaceless)},
      egl_surface{egl_display,
                  surfaceless ? EGL_NO_SURFACE : create_surface(egl_display, egl_config),
                  surfaceless ? EGLSurfaceStore::AllowNoSurface :
                                EGLSurfaceStore::DisallowNoSurface},
      egl_context{egl_display,
                  eglCreateContext(egl_display, egl_config,
                                   shared_context,
                                   default_egl_context_attr)}
{
}

void mgo::SurfacelessEGLContext::make_current() const
{
    if (eglGetCurrentContext() == egl_context)
        return;

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface,
                       egl_context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("could not make context current\n"));
    }
}

void mgo::SurfacelessEGLContext::release_current() const
{
    if (eglGetCurrentContext() == egl_context)
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

mgo::SurfacelessEGLContext::operator EGLContext() const
{
    return egl_context;
}
