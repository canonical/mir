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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "eglapp.h"
#include "mir_toolkit/mir_client_library.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <EGL/egl.h>

#include <xkbcommon/xkbcommon-keysyms.h>

static const char appname[] = "egldemo";

static MirConnection *connection;
static MirSurface *surface;
static EGLDisplay egldisplay;
static EGLSurface eglsurface;
static volatile sig_atomic_t running = 0;

#define CHECK(_cond, _err) \
    if (!(_cond)) \
    { \
        printf("%s\n", (_err)); \
        return 0; \
    }

void mir_eglapp_shutdown(void)
{
    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(egldisplay);
    mir_surface_release_sync(surface);
    surface = NULL;
    mir_connection_release(connection);
    connection = NULL;
}

static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

mir_eglapp_bool mir_eglapp_running(void)
{
    return running;
}

void mir_eglapp_swap_buffers(void)
{
    static time_t lasttime = 0;
    static int lastcount = 0;
    static int count = 0;
    time_t now = time(NULL);
    time_t dtime;
    int dcount;

    if (!running)
        return;

    eglSwapBuffers(egldisplay, eglsurface);

    count++;
    dcount = count - lastcount;
    dtime = now - lasttime;
    if (dtime)
    {
        printf("%d FPS\n", dcount);
        lasttime = now;
        lastcount = count;
    }
}

static void mir_eglapp_handle_input(MirSurface* surface, MirEvent const* ev, void* context)
{
    (void) surface;
    (void) context;
    if (ev->key.key_code == XKB_KEY_q && ev->key.action == mir_key_action_up)
        running = 0;
}

static unsigned int get_bpp(MirPixelFormat pf)
{
    switch (pf)
    {
        case mir_pixel_format_abgr_8888:
        case mir_pixel_format_xbgr_8888:
        case mir_pixel_format_argb_8888:
        case mir_pixel_format_xrgb_8888:
            return 32;
        case mir_pixel_format_bgr_888:
            return 24;
        case mir_pixel_format_invalid:
        default:
            return 0;
    }
}

static const MirDisplayOutput *find_active_output(
    const MirDisplayConfiguration *conf)
{
    const MirDisplayOutput *output = NULL;
    int d;

    for (d = 0; d < (int)conf->num_displays; d++)
    {
        const MirDisplayOutput *out = conf->displays + d;

        if (out->used &&
            out->connected &&
            out->num_modes &&
            out->current_mode < out->num_modes)
        {
            output = out;
            break;
        }
    }

    return output;
}

mir_eglapp_bool mir_eglapp_init(int argc, char *argv[],
                                unsigned int *width, unsigned int *height)
{
    EGLint ctxattribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    MirSurfaceParameters surfaceparm =
    {
        "eglappsurface",
        256, 256,
        mir_pixel_format_xbgr_8888,
        mir_buffer_usage_hardware
    };
    MirEventDelegate delegate = 
    {
        mir_eglapp_handle_input,
        NULL
    };
    EGLConfig eglconfig;
    EGLint neglconfigs;
    EGLContext eglctx;
    EGLBoolean ok;
    EGLint swapinterval = 1;

    if (argc > 1)
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            mir_eglapp_bool help = 0;
            const char *arg = argv[i];

            if (arg[0] == '-')
            {
                switch (arg[1])
                {
                case 'n':
                    swapinterval = 0;
                    break;
                case 'f':
                    *width = 0;
                    *height = 0;
                    break;
                case 's':
                    {
                        unsigned int w, h;
                        arg += 2;
                        if (!arg[0] && i < argc-1)
                        {
                            i++;
                            arg = argv[i];
                        }
                        if (sscanf(arg, "%ux%u", &w, &h) == 2)
                        {
                            *width = w;
                            *height = h;
                        }
                        else
                        {
                            printf("Invalid size: %s\n", arg);
                            help = 1;
                        }
                    }
                    break;
                case 'h':
                default:
                    help = 1;
                    break;
                }
            }
            else
            {
                help = 1;
            }

            if (help)
            {
                printf("Usage: %s [<options>]\n"
                       "  -f  Force full screen\n"
                       "  -h  Show this help text\n"
                       "  -n  Don't sync to vblank\n"
                       "  -s WIDTHxHEIGHT  Force surface size\n"
                       , argv[0]);
                return 0;
            }
        }
    }

    connection = mir_connect_sync(NULL, appname);
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    /* eglapps are interested in the screen size, so
       use mir_connection_create_display_config */
    MirDisplayConfiguration* display_config =
        mir_connection_create_display_config(connection);

    const MirDisplayOutput *output = find_active_output(display_config);

    if (output == NULL)
    {
        printf("No active outputs found.\n");
        return 0;
    }

    const MirDisplayMode *mode = &output->modes[output->current_mode];

    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection,
        &surfaceparm.pixel_format, 1, &valid_formats);

    printf("Connected to display: resolution (%dx%d), position(%dx%d), "
           "supports %d pixel formats\n",
           mode->horizontal_resolution, mode->vertical_resolution,
           output->position_x, output->position_y,
           output->num_output_formats);

    surfaceparm.width = *width > 0 ? *width : mode->horizontal_resolution;
    surfaceparm.height = *height > 0 ? *height : mode->vertical_resolution;

    mir_display_config_destroy(display_config);

    printf("Using pixel format #%d\n", surfaceparm.pixel_format);
    unsigned int bpp = get_bpp(surfaceparm.pixel_format);
    EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        EGL_BUFFER_SIZE, bpp,
        EGL_NONE
    };

    surface = mir_connection_create_surface_sync(connection, &surfaceparm);
    CHECK(mir_surface_is_valid(surface), "Can't create a surface");

    mir_surface_set_event_handler(surface, &delegate);

    egldisplay = eglGetDisplay(
                    mir_connection_get_egl_native_display(connection));
    CHECK(egldisplay != EGL_NO_DISPLAY, "Can't eglGetDisplay");

    ok = eglInitialize(egldisplay, NULL, NULL);
    CHECK(ok, "Can't eglInitialize");

    ok = eglChooseConfig(egldisplay, attribs, &eglconfig, 1, &neglconfigs);
    CHECK(ok, "Could not eglChooseConfig");
    CHECK(neglconfigs > 0, "No EGL config available");

    eglsurface = eglCreateWindowSurface(egldisplay, eglconfig,
            (EGLNativeWindowType)mir_surface_get_egl_native_window(surface),
            NULL);
    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT,
                              ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    *width = surfaceparm.width;
    *height = surfaceparm.height;

    eglSwapInterval(egldisplay, swapinterval);

    running = 1;

    return 1;
}

struct MirConnection* mir_eglapp_native_connection()
{
    return connection;
}

struct MirSurface* mir_eglapp_native_surface()
{
    return surface;
}
