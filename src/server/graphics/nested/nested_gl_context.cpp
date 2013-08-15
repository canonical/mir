
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

#include "nested_gl_context.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

static EGLint const dummy_pbuffer_attribs[] =
{
    EGL_WIDTH, 1,
    EGL_HEIGHT, 1,
    EGL_NONE
};

static const EGLint default_egl_context_attr [] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

namespace mgn = mir::graphics::nested;


mgn::NestedGLContext::NestedGLContext(EGLDisplay egl_display, EGLConfig egl_config, EGLContext egl_context) :
    egl_display{egl_display},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, egl_context, default_egl_context_attr)},
    egl_surface{egl_display, eglCreatePbufferSurface(egl_display, egl_config, dummy_pbuffer_attribs)}
{
}

mgn::NestedGLContext::~NestedGLContext() noexcept
{
    if (eglGetCurrentContext() != EGL_NO_CONTEXT)
        release_current();
}

void mgn::NestedGLContext::make_current()
{
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Mir Nested Display Error: Could not activate surface with eglMakeCurrent\n"));
}

void mgn::NestedGLContext::release_current()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}
