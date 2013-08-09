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

#ifndef NESTED_GL_CONTEXT_STORE_H_
#define NESTED_GL_CONTEXT_STORE_H_

#include <EGL/egl.h>

class EGLContextStore
{
public:
    EGLContextStore(EGLDisplay egl_display, EGLContext egl_context);
    virtual ~EGLContextStore() noexcept;

    operator EGLContext() const;

private:
    EGLContextStore(EGLContextStore const&) = delete;
    EGLContextStore& operator=(EGLContextStore const&) = delete;
    EGLDisplay const egl_display_;
    EGLContext const egl_context_;
};

#endif // NESTED_GL_CONTEXT_STORE_H_
