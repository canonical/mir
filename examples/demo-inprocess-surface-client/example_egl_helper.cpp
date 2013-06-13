/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "example_egl_helper.h"

#include <assert.h>

namespace me = mir::examples;

me::EGLHelper::EGLHelper(EGLNativeDisplayType native_display, EGLNativeWindowType native_window)
{
    display = eglGetDisplay(native_display);
    assert(display != EGL_NO_DISPLAY);

    int major, minor, rc;
    rc = eglInitialize(display, &major, &minor);
    assert(rc == EGL_TRUE);
    assert(major == 1);
    assert(minor == 4);

    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE };
    EGLConfig egl_config;
    int n;
    rc = eglChooseConfig(display, attribs, &egl_config, 1, &n);
    assert(rc == EGL_TRUE);
    assert(n == 1);
    (void)rc;

    surface = eglCreateWindowSurface(display, egl_config, native_window, nullptr);
    assert(surface != EGL_NO_SURFACE);

    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, egl_config, EGL_NO_CONTEXT, context_attribs);
    assert(surface != EGL_NO_CONTEXT);
}

me::EGLHelper::~EGLHelper()
{
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
}

EGLDisplay me::EGLHelper::the_display() const
{
    return display;
}

EGLContext me::EGLHelper::the_context() const
{
    return context;
}

EGLSurface me::EGLHelper::the_surface() const
{
    return surface;
}
