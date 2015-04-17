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

#include "mir/graphics/surfaceless_egl_context.h"
#include "mir/graphics/gl_extensions_base.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <vector>

namespace mg = mir::graphics;

namespace
{

class EGLExtensions : public mg::GLExtensionsBase
{
public:
    EGLExtensions(EGLDisplay egl_display) :
        mg::GLExtensionsBase{eglQueryString(egl_display, EGL_EXTENSIONS)}
    {
    }
};

bool supports_surfaceless_context(EGLDisplay egl_display)
{
    EGLExtensions const extensions{egl_display};

    return extensions.support("EGL_KHR_surfaceless_context");
}

std::vector<EGLint> ensure_pbuffer_set(EGLint const* attribs)
{
    bool has_preferred_surface = false;
    std::vector<EGLint> attribs_with_surface_type;
    int i = 0;

    while (attribs[i] != EGL_NONE)
    {
        attribs_with_surface_type.push_back(attribs[i]);
        if (attribs[i] == EGL_SURFACE_TYPE)
        {
            has_preferred_surface = true;
            if (attribs[i+1] == EGL_DONT_CARE)
            {
                /* Need to treat EGL_DONT_CARE specially, as it is defined as all-bits-set */
                attribs_with_surface_type.push_back(EGL_PBUFFER_BIT);
            }
            else
            {
                attribs_with_surface_type.push_back(attribs[i+1] | EGL_PBUFFER_BIT);
            }
        }
        else
        {
            attribs_with_surface_type.push_back(attribs[i+1]);
        }
        i += 2;
    }

    if (!has_preferred_surface)
    {
        attribs_with_surface_type.push_back(EGL_SURFACE_TYPE);
        attribs_with_surface_type.push_back(EGL_PBUFFER_BIT);
    }

    attribs_with_surface_type.push_back(EGL_NONE);

    return attribs_with_surface_type;
}

EGLConfig choose_config(EGLDisplay egl_display, EGLint const* attribs, bool surfaceless)
{
    EGLConfig egl_config{0};
    int num_egl_configs{0};
    std::vector<EGLint> validated_attribs;

    if (!surfaceless)
    {
        validated_attribs = ensure_pbuffer_set(attribs);
        attribs = validated_attribs.data();
    }

    if (eglChooseConfig(egl_display, attribs, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose EGL config"));
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

EGLint const default_attr[] =
{
    EGL_SURFACE_TYPE, EGL_DONT_CARE,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 0,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};


}

mg::SurfacelessEGLContext::SurfacelessEGLContext(EGLDisplay egl_display, EGLContext shared_context)
    : SurfacelessEGLContext(egl_display, default_attr, shared_context)
{
}

mg::SurfacelessEGLContext::SurfacelessEGLContext(
        EGLDisplay egl_display,
        EGLint const* attribs,
        EGLContext shared_context)
    : egl_display{egl_display},
      surfaceless{supports_surfaceless_context(egl_display)},
      egl_config{choose_config(egl_display, attribs, surfaceless)},
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


mg::SurfacelessEGLContext::SurfacelessEGLContext(SurfacelessEGLContext&& move)
    : egl_display(move.egl_display),
      surfaceless(move.surfaceless),
      egl_config(move.egl_config),
      egl_surface{std::move(move.egl_surface)},
      egl_context{std::move(move.egl_context)}
{
    move.egl_display = EGL_NO_DISPLAY;
}

mg::SurfacelessEGLContext::~SurfacelessEGLContext() noexcept
{
    release_current();
}


void mg::SurfacelessEGLContext::make_current() const
{
    if (eglGetCurrentContext() == egl_context)
        return;

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface,
                       egl_context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(
            mg::egl_error("could not make context current"));
    }
}

void mg::SurfacelessEGLContext::release_current() const
{
    if (egl_context != EGL_NO_CONTEXT && eglGetCurrentContext() == egl_context)
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

mg::SurfacelessEGLContext::operator EGLContext() const
{
    return egl_context;
}
