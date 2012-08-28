/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 * Thomas Voss <thomas.voss@canonical.com>
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test/egl_mock.h"
#include <gtest/gtest.h>
namespace
{
mir::EglMock* global_mock = NULL;
}

mir::EglMock::EglMock()
    : fake_egl_display((EGLDisplay) 0x0530)
{
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, eglGetDisplay(_))
        .WillByDefault(Return(fake_egl_display));
    ON_CALL(*this, eglInitialize(_,_,_))
        .WillByDefault(DoAll(
                    SetArgPointee<1>(1),
                    SetArgPointee<2>(4),
                    Return(EGL_TRUE)));

}

mir::EglMock::~EglMock()
{
    global_mock = NULL;
}

#define CHECK_GLOBAL_MOCK(rettype)  \
    if (!global_mock)               \
    {                               \
        using namespace ::testing;  \
        ADD_FAILURE_AT("__FILE__",__LINE__); \
        rettype type = (rettype) 0;       \
        return type;        \
    }

EGLint eglGetError (void)
{
    CHECK_GLOBAL_MOCK(EGLint)
    return global_mock->eglGetError();
}

EGLDisplay eglGetDisplay (NativeDisplayType display)
{
    CHECK_GLOBAL_MOCK(EGLDisplay);
    return global_mock->eglGetDisplay(display);
}

EGLBoolean eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglInitialize(dpy, major, minor);
}

EGLBoolean eglTerminate (EGLDisplay dpy)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglTerminate(dpy);
}

const char * eglQueryString (EGLDisplay dpy, EGLint name)
{
    CHECK_GLOBAL_MOCK(const char *)
    return global_mock->eglQueryString(dpy, name);
}

EGLBoolean eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglGetConfigs(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLBoolean eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglGetConfigAttrib(dpy, config, attribute, value);
}

EGLSurface eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock->eglCreateWindowSurface(dpy, config, window, attrib_list);
}

EGLSurface eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock->eglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLSurface eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock->eglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLBoolean eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglDestroySurface(dpy, surface);
}

EGLBoolean eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglQuerySurface(dpy, surface, attribute, value);
}

/* EGL 1.1 render-to-texture APIs */
EGLBoolean eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglReleaseTexImage(dpy, surface, buffer);
}

/* EGL 1.1 swap control API */
EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglSwapInterval(dpy, interval);
}

EGLContext eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLContext)
    return global_mock->eglCreateContext(dpy, config, share_list, attrib_list);
}

EGLBoolean eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglDestroyContext(dpy, ctx);
}

EGLBoolean eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglMakeCurrent(dpy, draw, read, ctx);
}

EGLContext eglGetCurrentContext (void)
{
    CHECK_GLOBAL_MOCK(EGLContext)
    return global_mock->eglGetCurrentContext();
}

EGLSurface eglGetCurrentSurface (EGLint readdraw)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock->eglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetCurrentDisplay (void)
{
    CHECK_GLOBAL_MOCK(EGLDisplay)
    return global_mock->eglGetCurrentDisplay();
}

EGLBoolean eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglQueryContext(dpy, ctx, attribute, value);
}

EGLBoolean eglWaitGL (void)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglWaitGL();
}

EGLBoolean eglWaitNative (EGLint engine)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglWaitNative(engine);
}

EGLBoolean eglSwapBuffers (EGLDisplay dpy, EGLSurface draw)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglSwapBuffers(dpy, draw);
}

EGLBoolean eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, NativePixmapType target)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock->eglCopyBuffers(dpy, surface, target);
}

