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

#include "mir/graphics/egl_resources.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cstring>

namespace mg = mir::graphics;

/*******************
 * EGLContextStore *
 *******************/

mg::EGLContextStore::EGLContextStore(EGLDisplay egl_display, EGLContext egl_context)
    : egl_display_{egl_display}, egl_context_{egl_context}
{
    if (egl_context_ == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not create egl context\n"));
}

mg::EGLContextStore::~EGLContextStore() noexcept
{
    if (egl_context_ != EGL_NO_CONTEXT)
        eglDestroyContext(egl_display_, egl_context_);
}

mg::EGLContextStore::EGLContextStore(EGLContextStore&& other)
    : egl_display_{other.egl_display_},
      egl_context_{other.egl_context_}
{
    other.egl_display_ = EGL_NO_DISPLAY;
    other.egl_context_ = EGL_NO_CONTEXT;
}

mg::EGLContextStore::operator EGLContext() const
{
    return egl_context_;
}

/*******************
 * EGLSurfaceStore *
 *******************/

mg::EGLSurfaceStore::EGLSurfaceStore(EGLDisplay egl_display, EGLSurface egl_surface,
                                     enum AllowNoSurface allow_no_surface)
    : egl_display_{egl_display}, egl_surface_{egl_surface}
{
    if (egl_surface_ == EGL_NO_SURFACE && !allow_no_surface)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not create egl surface\n"));
}

mg::EGLSurfaceStore::EGLSurfaceStore(EGLDisplay egl_display, EGLSurface egl_surface)
    : EGLSurfaceStore(egl_display, egl_surface, DisallowNoSurface)
{
}

mg::EGLSurfaceStore::EGLSurfaceStore(EGLSurfaceStore&& other)
    : egl_display_{other.egl_display_},
      egl_surface_{other.egl_surface_}
{
    other.egl_display_ = EGL_NO_DISPLAY;
    other.egl_surface_ = EGL_NO_SURFACE;
}

mg::EGLSurfaceStore mg::EGLSurfaceStore::create_dummy_surface(EGLDisplay egl_display, EGLConfig egl_config)
{
    EGLint const dummy_pbuffer_attribs[] =
    {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };

    /* If we have surfaceless support then we don't need a real surface */
    if (strstr(eglQueryString(egl_display, EGL_EXTENSIONS), "EGL_KHR_surfaceless_context") != nullptr)
        return EGLSurfaceStore(egl_display, EGL_NO_SURFACE, AllowNoSurface);

    return EGLSurfaceStore(egl_display, eglCreatePbufferSurface(egl_display, egl_config, dummy_pbuffer_attribs));
}

mg::EGLSurfaceStore::~EGLSurfaceStore() noexcept
{
    if (egl_surface_ != EGL_NO_SURFACE)
        eglDestroySurface(egl_display_, egl_surface_);
}

mg::EGLSurfaceStore::operator EGLSurface() const
{
    return egl_surface_;
}
