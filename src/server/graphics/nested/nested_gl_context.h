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

#ifndef NESTED_GL_CONTEXT_H_
#define NESTED_GL_CONTEXT_H_

#include "mir/graphics/gl_context.h"
#include "nested_gl_context_store.h"
#include "nested_surface_store.h"

#include <EGL/egl.h>

namespace mg = mir::graphics;

class NestedGLContext : public mg::GLContext
{
public:
    NestedGLContext(EGLDisplay egl_display, EGLConfig egl_config, EGLContext egl_context);
    virtual ~NestedGLContext() noexcept;

    void make_current();
    void release_current();

private:
    EGLDisplay const egl_display;
    EGLContextStore const egl_context;
    EGLSurfaceStore const egl_surface;
};

#endif // NESTED_GL_CONTEXT_H_

