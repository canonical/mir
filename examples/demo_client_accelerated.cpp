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
 * Authored by: Kevin DuBois   <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir/draw/graphics.h"

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

static char const *socket_file = "/tmp/mir_socket";
static MirConnection *connection = 0;
static MirSurface *surface = 0;

static void set_connection(MirConnection *new_connection, void * context)
{
    (void)context;
    connection = new_connection;
}

static void surface_create_callback(MirSurface *new_surface, void *context)
{
    (void)context;
    surface = new_surface;
}

static void surface_release_callback(MirSurface *old_surface, void *context)
{
    (void)old_surface;
    (void)context;
    surface = 0;
}

int main(int argc, char* argv[])
{
    int arg;
    opterr = 0;
    while ((arg = getopt (argc, argv, "hf:")) != -1)
    {
        switch (arg)
        {
        case 'f':
            socket_file = optarg;
            break;

        case '?':
        case 'h':
        default:
            puts(argv[0]);
            puts("Usage:");
            puts("    -f <socket filename>");
            puts("    -h: this help text");
            return -1;
        }
    }

    puts("Starting");

    mir_wait_for(mir_connect(socket_file, __PRETTY_FUNCTION__, set_connection, 0));
    puts("Connected");

    assert(connection != NULL);
//    assert(mir_connection_is_valid(connection));
//    assert(strcmp(mir_connection_get_error_message(connection), "") == 0);

    MirDisplayInfo display_info;
    mir_connection_get_display_info(connection, &display_info);
    assert(display_info.supported_pixel_format_items > 0);

    MirPixelFormat pixel_format = display_info.supported_pixel_format[0];

    MirSurfaceParameters const request_params =
        {__PRETTY_FUNCTION__, 640, 480, pixel_format, mir_buffer_usage_hardware};
    mir_wait_for(mir_surface_create(connection, &request_params, surface_create_callback, 0));
    puts("Surface created");

    assert(surface != NULL);
    assert(mir_surface_is_valid(surface));
    assert(strcmp(mir_surface_get_error_message(surface), "") == 0);

    /* egl setup */
    int major, minor, n, rc;
    EGLDisplay disp;
    EGLContext context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE };
    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    EGLNativeDisplayType native_display = (EGLNativeDisplayType) mir_connection_get_egl_native_display(connection);
    EGLNativeWindowType native_window = (EGLNativeWindowType) mir_surface_get_egl_native_window(surface);
    assert(native_window != NULL);

    disp = eglGetDisplay(native_display);
    assert(disp != EGL_NO_DISPLAY);

    rc = eglInitialize(disp, &major, &minor);
    assert(rc == EGL_TRUE);
    assert(major == 1);
    assert(minor == 4);

    rc = eglChooseConfig(disp, attribs, &egl_config, 1, &n);
    assert(rc == EGL_TRUE);
    assert(n == 1);

    egl_surface = eglCreateWindowSurface(disp, egl_config, native_window, NULL);
    assert(egl_surface != EGL_NO_SURFACE);

    context = eglCreateContext(disp, egl_config, EGL_NO_CONTEXT, context_attribs);
    assert(egl_surface != EGL_NO_CONTEXT);

    rc = eglMakeCurrent(disp, egl_surface, egl_surface, context);
    assert(rc == EGL_TRUE);

    mir::draw::glAnimationBasic gl_animation;
    gl_animation.init_gl();

    for(;;)
    {
        gl_animation.render_gl();
        rc = eglSwapBuffers(disp, egl_surface);
        assert(rc == EGL_TRUE);

        usleep(167);//60fps

        gl_animation.step();
    }

    eglDestroySurface(disp, egl_surface);
    eglDestroyContext(disp, context);
    eglTerminate(disp);

    mir_wait_for(mir_surface_release(surface, surface_release_callback, 0));
    puts("Surface released");

    mir_connection_release(connection);
    puts("Connection released");

    return 0;
}



