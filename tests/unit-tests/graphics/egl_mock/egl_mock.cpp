#include "egl_mock.h"

namespace
{
mir::EglMock* global_mock = NULL;
}

mir::EglMock::EglMock()
{
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;
}

mir::EglMock::~EglMock()
{
    global_mock = NULL;
}


EGLint eglGetError (void)
{
    if (global_mock)
    {
        return global_mock->eglGetError();
    }
}

EGLDisplay eglGetDisplay (NativeDisplayType display)
{
    if (global_mock)
    {
        return global_mock->eglGetDisplay(display);
    }
}

EGLBoolean eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    if (global_mock)
    {
        return global_mock->eglInitialize(dpy, major, minor);
    }
}

EGLBoolean eglTerminate (EGLDisplay dpy)
{
    if (global_mock)
    {
        return global_mock->eglTerminate(dpy);
    }
}

const char * eglQueryString (EGLDisplay dpy, EGLint name)
{
    if (global_mock)
    {
        return global_mock->eglQueryString(dpy, name);
    }
}

EGLBoolean eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    if (global_mock)
    {
        return global_mock->eglGetConfigs(dpy, configs, config_size, num_config);
    }
}

EGLBoolean eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    if (global_mock)
    {
        return global_mock->eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
    }
}

EGLBoolean eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    if (global_mock)
        return global_mock->eglGetConfigAttrib(dpy, config, attribute, value);
}

EGLSurface eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
    if (global_mock)
        return global_mock->eglCreateWindowSurface(dpy, config, window, attrib_list);
}

EGLSurface eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
    if (global_mock)
        return global_mock->eglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLSurface eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    if (global_mock)
        return global_mock->eglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLBoolean eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    if (global_mock)
        return global_mock->eglDestroySurface(dpy, surface);
}

EGLBoolean eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    if (global_mock)
        return global_mock->eglQuerySurface(dpy, surface, attribute, value);
}

/* EGL 1.1 render-to-texture APIs */
EGLBoolean eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    if (global_mock)
        return global_mock->eglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    if (global_mock)
        return global_mock->eglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    if (global_mock)
        return global_mock->eglReleaseTexImage(dpy, surface, buffer);
}

/* EGL 1.1 swap control API */
EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    if (global_mock)
        return global_mock->eglSwapInterval(dpy, interval);
}

EGLContext eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
    if (global_mock)
        return global_mock->eglCreateContext(dpy, config, share_list, attrib_list);
}

EGLBoolean eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    if (global_mock)
        return global_mock->eglDestroyContext(dpy, ctx);
}

EGLBoolean eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    if (global_mock)
        return global_mock->eglMakeCurrent(dpy, draw, read, ctx);
}

EGLContext eglGetCurrentContext (void)
{
    if (global_mock)
        return global_mock->eglGetCurrentContext();
}

EGLSurface eglGetCurrentSurface (EGLint readdraw)
{
    if (global_mock)
        return global_mock->eglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetCurrentDisplay (void)
{
    if (global_mock)
        return global_mock->eglGetCurrentDisplay();
}

EGLBoolean eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    if (global_mock)
        return global_mock->eglQueryContext(dpy, ctx, attribute, value);
}

EGLBoolean eglWaitGL (void)
{
    if (global_mock)
        return global_mock->eglWaitGL();
}

EGLBoolean eglWaitNative (EGLint engine)
{
    if (global_mock)
        return global_mock->eglWaitNative(engine);
}

EGLBoolean eglSwapBuffers (EGLDisplay dpy, EGLSurface draw)
{
    if (global_mock)
        return global_mock->eglSwapBuffers(dpy, draw);
}

EGLBoolean eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, NativePixmapType target)
{
    if (global_mock)
        return global_mock->eglCopyBuffers(dpy, surface, target);
}

