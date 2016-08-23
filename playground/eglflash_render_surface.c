/*
 * Trivial GL demo; flashes the screen with colors using a MirRenderSurface.
 *
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include <mir_toolkit/mir_client_library.h>
#include "mir_toolkit/mir_render_surface.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <pthread.h>

typedef struct Color
{
    GLfloat r, g, b, a;
} Color;

#define CHECK(_cond, _err) \
    if (!(_cond)) \
    { \
       printf("%s\n", (_err)); \
       return -1; \
    }

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    unsigned int num_frames = 0;
    const char* appname = "EGL Render Surface Demo";
    int width = 100;
    int height = 100;
    EGLDisplay egldisplay;
    EGLSurface eglsurface;
    EGLint ctxattribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext eglctx;
    EGLConfig eglconfig;
    EGLint neglconfigs;
    EGLBoolean ok;
    MirConnection* connection = NULL;
    MirSurface* surface = NULL;
    MirRenderSurface* render_surface = NULL;

    connection = mir_connect_sync(NULL, appname);
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    egldisplay = eglGetDisplay(
                    mir_connection_get_egl_native_display(connection));
    CHECK(egldisplay != EGL_NO_DISPLAY, "Can't eglGetDisplay");

    ok = eglInitialize(egldisplay, NULL, NULL);
    CHECK(ok, "Can't eglInitialize");

    const EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    ok = eglChooseConfig(egldisplay, attribs, &eglconfig, 1, &neglconfigs);
    CHECK(ok, "Could not eglChooseConfig");
    CHECK(neglconfigs > 0, "No EGL config available");

    MirPixelFormat pixel_format =
        mir_connection_get_egl_pixel_format(connection, egldisplay, eglconfig);

    printf("Mir chose pixel format %d.\n", pixel_format);

    MirSurfaceSpec *spec =
        mir_connection_create_spec_for_normal_surface(connection, width, height, pixel_format);
    CHECK(spec, "Can't create a surface spec");

    mir_surface_spec_set_name(spec, appname);

    render_surface = mir_connection_create_render_surface(connection, width, height, pixel_format);
    CHECK(mir_render_surface_is_valid(connection, render_surface), "could not create render surface");

    mir_surface_spec_add_render_surface(spec, width, height, 0, 0, render_surface);

    surface = mir_surface_create_sync(spec);

    mir_surface_spec_release(spec);

    eglsurface = eglCreateWindowSurface(egldisplay, eglconfig,
        (EGLNativeWindowType)render_surface, NULL);
    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT,
                              ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    Color red = {1.0f, 0.0f, 0.0f, 1.0f};
    Color green = {0.0f, 1.0f, 0.0f, 1.0f};
    Color blue = {0.0f, 0.0f, 1.0f, 1.0f};

    do
    {
        glClearColor(red.r, red.g, red.b, red.a);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(egldisplay, eglsurface);
        sleep(1);

        glClearColor(green.r, green.g, green.b, green.a);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(egldisplay, eglsurface);
        sleep(1);

        glClearColor(blue.r, blue.g, blue.b, blue.a);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(egldisplay, eglsurface);
        sleep(1);
    } while (num_frames++ < 3);

    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(egldisplay);
    mir_render_surface_release(connection, render_surface);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);

    return 0;
}
