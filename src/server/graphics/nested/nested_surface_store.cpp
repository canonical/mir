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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "nested_surface_store.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

EGLSurfaceStore::EGLSurfaceStore(EGLDisplay egl_display, EGLSurface egl_surface) :
    egl_display_{egl_display},
    egl_surface_{egl_surface}
{
    if (egl_surface_ == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Display: Could not create egl surface\n"));
}

EGLSurfaceStore::~EGLSurfaceStore()
{
    eglDestroySurface(egl_display_, egl_surface_);
}

EGLSurfaceStore::operator EGLSurface() const
{
    return egl_surface_;
}
