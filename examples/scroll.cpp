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
#include "graphics.h"

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

static char const *socket_file = NULL;

int main(int argc, char* argv[])
{
    MirConnection *connection = 0;
    MirSurface *surface = 0;
    int arg;
    opterr = 0;
    while ((arg = getopt (argc, argv, "hm:")) != -1)
    {
        switch (arg)
        {
        case 'm':
            socket_file = optarg;
            break;

        case '?':
        case 'h':
        default:
            puts(argv[0]);
            puts("Usage:");
            puts("    -m <Mir server socket>");
            puts("    -h: this help text");
            return -1;
        }
    }

    puts("Starting");

    connection = mir_connect_sync(socket_file, __PRETTY_FUNCTION__);
    assert(connection != NULL);
    assert(mir_connection_is_valid(connection));
    assert(strcmp(mir_connection_get_error_message(connection), "") == 0);
    puts("Connected");

    MirPixelFormat pixel_format;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);

    MirSurfaceParameters const request_params =
        {__PRETTY_FUNCTION__, 640, 480, pixel_format,
         mir_buffer_usage_hardware, mir_display_output_id_invalid};

    surface = mir_connection_create_surface_sync(connection, &request_params);
    assert(surface != NULL);
    assert(mir_surface_is_valid(surface));
    assert(strcmp(mir_surface_get_error_message(surface), "") == 0);
    puts("Surface created");

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
    assert(context != EGL_NO_CONTEXT);

    rc = eglMakeCurrent(disp, egl_surface, egl_surface, context);
    assert(rc == EGL_TRUE);

    mir::draw::glAnimationBasic gl_animation;
    gl_animation.init_gl();

    for(;;)
    {
        gl_animation.render_gl();
        rc = eglSwapBuffers(disp, egl_surface);
        assert(rc == EGL_TRUE);
        gl_animation.step();
    }

    eglDestroySurface(disp, egl_surface);
    eglDestroyContext(disp, context);
    eglTerminate(disp);

    mir_surface_release_sync(surface);
    puts("Surface released");

    mir_connection_release(connection);
    puts("Connection released");

    (void)rc;

    return 0;
}



