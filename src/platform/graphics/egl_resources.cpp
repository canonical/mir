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
    eglDestroyContext(egl_display_, egl_context_);
}

mg::EGLContextStore::operator EGLContext() const
{
    return egl_context_;
}

/*******************
 * EGLSurfaceStore *
 *******************/

mg::EGLSurfaceStore::EGLSurfaceStore(EGLDisplay egl_display, EGLSurface egl_surface)
        : egl_display_{egl_display}, egl_surface_{egl_surface}
{
    if (egl_surface_ == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not create egl surface\n"));
}

mg::EGLSurfaceStore::~EGLSurfaceStore() noexcept
{
    eglDestroySurface(egl_display_, egl_surface_);
}

mg::EGLSurfaceStore::operator EGLSurface() const
{
    return egl_surface_;
}
