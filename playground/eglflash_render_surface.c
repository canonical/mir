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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *         Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_render_surface.h"

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

void resize_callback(
    MirSurface*, MirEvent const* event, void* context)
{
    MirEventType type = mir_event_get_type(event);
    if (type == mir_event_type_resize)
    {
        MirResizeEvent const* resize_event = mir_event_get_resize_event(event);
        int width = mir_resize_event_get_width(resize_event);
        int height = mir_resize_event_get_height(resize_event);
        MirRenderSurface* rs = (MirRenderSurface*) context;
        mir_render_surface_set_logical(rs, width, height);
    }
}

struct DriverInfo
{
    MirConnection* connection;
    MirRenderSurface* surface;
    MirBufferStream* stream;
    int current_width;
    int current_height;
};

//note that this function only has information available to the driver at the time
//that eglCreateWindowSurface will be called.
EGLSurface future_driver_eglCreateWindowSurface(
    DriverInfo* info,
    EGLDisplay display, EGLConfig config, MirRenderSurface* surface) //available from signature of EGLCreateWindowSurface
{
    //TODO: the driver needs to be selecting a pixel format that's acceptable based on
    //      the EGLConfig. mir_connection_get_egl_pixel_format
    //      needs to be deprecated once the drivers support the Mir EGL platform.
    MirPixelFormat pixel_format = mir_connection_get_egl_pixel_format(info->connection, display, config);
    info->surface = surface;

    int width = -1;
    int height = -1;
    mir_render_surface_logical_size(surface, info->current_width, info->current_height);

    printf("The driver chose pixel format %d.\n", pixel_format);
    //this particular [silly] driver has chosen the buffer stream as the way it wants to post
    //its hardware content. I'd think most drivers would want MirPresentationChain for flexibility
    info->stream = mir_render_surface_create_buffer_stream_sync(
        surface,
        info->current_width, info->current_height,
        pixel_format,
        mir_buffer_usage_hardware);

    return eglCreateWindowSurface(display, config, (EGLNativeWindowType) surface, NULL);
}

void future_driver_eglSwapBuffers(DriverInfo* info, EGLDisplay display, EGLSurface surface)
{
    int width = -1;
    int height = -1;
    mir_render_surface_get_logical_size(info.surface, &width, &height);
    if (width != info->current_width || height != info->current_height)
    {
        mir_buffer_stream_set_size(info->size, width, height);
        info->current_width = width;
        info->current_height = height;
    } 
    eglSwapBuffers(display, surface);
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
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

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    connection = mir_connect_sync(NULL, appname);
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    //FIXME: this should be:
    //egldislay = eglGetDisplay(connection);
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

    //The format field is only used for default-created streams.
    //We can safely set invalid as the pixel format, and the field needs to be deprecated
    //once default streams are deprecated. 
    MirSurfaceSpec *spec =
        mir_connection_create_spec_for_normal_surface(
            connection, width, height,
            mir_pixel_format_invalid);

    CHECK(spec, "Can't create a surface spec");
    mir_surface_spec_set_name(spec, appname);

    render_surface = mir_connection_create_render_surface(connection, width, height);
    CHECK(mir_render_surface_is_valid(render_surface), "could not create render surface");
    mir_surface_spec_add_render_surface(spec, render_surface, width, height, 0, 0);

    mir_surface_spec_set_event_handler(spec, event_callback, render_surface);

    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    eglsurface = future_driver_eglCreateWindowSurface(
        connection, egldisplay, eglconfig, render_surface);

    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT,
                              ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    Color red = {1.0f, 0.0f, 0.0f, 1.0f};
    Color green = {0.0f, 1.0f, 0.0f, 1.0f};
    Color blue = {0.0f, 0.0f, 1.0f, 1.0f};

    running = 1;
    while (running)
    {
        glClearColor(red.r, red.g, red.b, red.a);
        glClear(GL_COLOR_BUFFER_BIT);
        future_driver_eglSwapBuffers(egldisplay, eglsurface, render_surface);
        sleep(1);

        glClearColor(green.r, green.g, green.b, green.a);
        glClear(GL_COLOR_BUFFER_BIT);
        future_driver_eglSwapBuffers(egldisplay, eglsurface, render_surface);
        sleep(1);

        glClearColor(blue.r, blue.g, blue.b, blue.a);
        glClear(GL_COLOR_BUFFER_BIT);
        future_driver_eglSwapBuffers(egldisplay, eglsurface, render_surface);
        sleep(1);
    }

    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(egldisplay);
    mir_render_surface_release(render_surface);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);

    return 0;
}
