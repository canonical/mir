/*
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *         Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *         Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_render_surface.h"
#include "mir_egl_platform_shim.h"
#include "diamond.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

static volatile sig_atomic_t running = 0;

static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

#define CHECK(_cond, _err) \
    if (!(_cond)) \
    { \
       printf("%s\n", (_err)); \
       return -1; \
    }

//The client arranges the scene in the subscene
void resize_callback(MirSurface* surface, MirEvent const* event, void* context)
{
    (void) surface;
    MirEventType type = mir_event_get_type(event);
    if (type == mir_event_type_resize)
    {
        MirResizeEvent const* resize_event = mir_event_get_resize_event(event);
        int width = mir_resize_event_get_width(resize_event);
        int height = mir_resize_event_get_height(resize_event);
        MirRenderSurface* rs = (MirRenderSurface*) context;
        mir_render_surface_set_logical_size(rs, width, height);
    }
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    const char* appname = "EGL Render Surface Demo";
    int width = 300;
    int height = 300;
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

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    connection = mir_connect_sync(NULL, appname);
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    egldisplay = future_driver_eglGetDisplay(connection);

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

    render_surface = mir_connection_create_render_surface(connection, width, height);
    CHECK(mir_render_surface_is_valid(render_surface), "could not create render surface");

    //FIXME: we should be able to eglCreateWindowSurface or mir_surface_create in any order.
    //       Current code requires creation of egl window before mir surface.
    eglsurface = future_driver_eglCreateWindowSurface(egldisplay, eglconfig, render_surface);

    //The format field is only used for default-created streams.
    //We can safely set invalid as the pixel format, and the field needs to be deprecated
    //once default streams are deprecated.
    //width and height are the logical width the user wants the surface to be
    MirSurfaceSpec *spec =
        mir_connection_create_spec_for_normal_surface(
            connection, width, height,
            mir_pixel_format_invalid);

    CHECK(spec, "Can't create a surface spec");
    mir_surface_spec_set_name(spec, appname);

    mir_surface_spec_add_render_surface(spec, render_surface, width, height, 0, 0);

    mir_surface_spec_set_event_handler(spec, resize_callback, render_surface);

    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT,
                              ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    Diamond diamond = setup_diamond(width, height);

    running = 1;
    while (running)
    {
        render_diamond(&diamond, egldisplay, eglsurface);
        future_driver_eglSwapBuffers(egldisplay, eglsurface);
    }

    destroy_diamond(&diamond);

    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    future_driver_eglTerminate(egldisplay);
    mir_render_surface_release(render_surface);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);

    return 0;
}
