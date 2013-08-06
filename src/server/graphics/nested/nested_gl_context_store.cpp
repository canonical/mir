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

#include "nested_gl_context_store.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

EGLContextStore::EGLContextStore(EGLDisplay egl_display, EGLContext egl_context) :
    egl_display_{egl_display},
    egl_context_{egl_context}
{
    if (egl_context_ == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("Mir Nested Display Error: Could not create EGL context."));
}

EGLContextStore::~EGLContextStore()
{
    eglDestroyContext(egl_display_, egl_context_);
}

EGLContextStore::operator EGLContext() const
{
    return egl_context_;
}
