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
#include <string.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <xkbcommon/xkbcommon-keysyms.h>

float mir_eglapp_background_opacity = 1.0f;

static const char* appname = "egldemo";

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
    EGLint width, height;

    if (!running)
        return;

    eglSwapBuffers(egldisplay, eglsurface);

    /*
     * Querying the surface (actually the current buffer) dimensions here is
     * the only truly safe way to be sure that the dimensions we think we
     * have are those of the buffer being rendered to. But this should be
     * improved in future; https://bugs.launchpad.net/mir/+bug/1194384
     */
    if (eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &width) &&
        eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &height))
    {
        glViewport(0, 0, width, height);
    }
}

static void mir_eglapp_handle_input_event(MirInputEvent const* event)
{
    if (mir_input_event_get_type(event) != mir_input_event_type_key)
        return;
    MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(event);
    if (mir_keyboard_event_action(kev) != mir_keyboard_action_up)
        return;
    if (mir_keyboard_event_key_code(kev) != XKB_KEY_q)
        return;
    
    running = 0;
}

static void mir_eglapp_handle_surface_event(MirSurfaceEvent const* sev)
{
    MirSurfaceAttrib attrib = mir_surface_event_get_attribute(sev);
    if (attrib != mir_surface_attrib_visibility)
        return;
    switch (mir_surface_event_get_attribute_value(sev))
    {
    case mir_surface_visibility_exposed:
        printf("Surface exposed\n");
        break;
    case mir_surface_visibility_occluded:
        printf("Surface occluded\n");
        break;
    default:
        break;
    }
}

static void mir_eglapp_handle_event(MirSurface* surface, MirEvent const* ev, void* context)
{
    (void) surface;
    (void) context;
    
    switch (mir_event_get_type(ev))
    {
    case mir_event_type_input:
        mir_eglapp_handle_input_event(mir_event_get_input_event(ev));
        break;
    case mir_event_type_surface:
        mir_eglapp_handle_surface_event(mir_event_get_surface_event(ev));
        break;
    case mir_event_type_resize:
        /*
         * FIXME: https://bugs.launchpad.net/mir/+bug/1194384
         * It is unsafe to set the width and height here because we're in a
         * different thread to that doing the rendering. So we either need
         * support for event queuing (directing them to another thread) or
         * full single-threaded callbacks. (LP: #1194384).
         */
        {
            MirResizeEvent const* resize = mir_event_get_resize_event(ev);
            printf("Resized to %dx%d\n",
                   mir_resize_event_get_width(resize),
                   mir_resize_event_get_height(resize));
        }
        break;
    case mir_event_type_close_surface:
        printf("Received close event from server.\n");
        running = 0;
        break;
    default:
        break;
    }
}

static const MirDisplayOutput *find_active_output(
    const MirDisplayConfiguration *conf)
{
    const MirDisplayOutput *output = NULL;
    int d;

    for (d = 0; d < (int)conf->num_outputs; d++)
    {
        const MirDisplayOutput *out = conf->outputs + d;

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

struct mir_eglapp_arg
{
    char const* arg;
    char const* scanf_desc;
    char const* scanf_format;  /* or "" bool flag, or "=" for argv copy */
    void* variable;
    char const* desc;
};

static void show_help(const struct mir_eglapp_arg* const* arg_lists)
{
    const struct mir_eglapp_arg* const* list = NULL;
    int max_len = 0;
    int const indent = 2;
    int const desc_offset = 2;

    for (list = arg_lists; *list != NULL; ++list)
    {
        const struct mir_eglapp_arg* arg = *list;
        for (; arg->arg != NULL; ++arg)
        {
            int len = indent + strlen(arg->arg) + 1 + strlen(arg->scanf_desc);
            if (len > max_len)
                max_len = len;
        }
    }
    for (list = arg_lists; *list != NULL; ++list)
    {
        const struct mir_eglapp_arg* arg = *list;
        for (; arg->arg != NULL; ++arg)
        {
            int len = 0, remainder = 0;
            printf("%*c%s %s%n", indent, ' ', arg->arg, arg->scanf_desc, &len);
            remainder = desc_offset + max_len - len;
            /* TODO: Implement line wrapping for long descriptions? */
            printf("%*c%s\n", remainder, ' ', arg->desc);
        }
    }
}

static mir_eglapp_bool parse_args(int argc, char *argv[],
                             const struct mir_eglapp_arg* const* arg_lists)
{
    for (int i = 1; i < argc; ++i)
    {
        const struct mir_eglapp_arg* const* list;
        for (list = arg_lists; *list != NULL; ++list)
        {
            const struct mir_eglapp_arg* arg;
            for (arg = *list; arg->arg != NULL; ++arg)
            {
                if (!strncmp(argv[i], arg->arg, strlen(arg->arg)))
                {
                    int matched = 1;
                    if (arg->scanf_format[0] && i+1 >= argc)
                    {
                        fprintf(stderr, "Missing argument for %s\n", argv[i]);
                        return 0;
                    }
                    switch (arg->scanf_format[0])
                    {
                    case '\0':
                        if (arg->variable == NULL)  /* "--" */ 
                            return 1;
                        *(mir_eglapp_bool*)arg->variable = 1;
                        break;
                    case '=':
                        *(char const**)arg->variable = argv[++i];
                        break;
                    case '%':
                        matched = sscanf(argv[++i], arg->scanf_format,
                                         arg->variable);
                        break;
                    default:
                        matched = 0;
                        break;
                    }
                    if (!matched)
                    {
                        fprintf(stderr, "Invalid option: %s %s  (expected %s)\n",
                                arg->arg, argv[i], arg->scanf_desc);
                        return 0;
                    }
                    goto parsed;
                }
            }
        }
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return 0;
        parsed:
        (void)0; /* Stop compiler warnings about label at the end */
    }
    return 1;
}

mir_eglapp_bool mir_eglapp_init(int argc, char *argv[],
                                unsigned int *width, unsigned int *height)
{
    EGLint ctxattribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLConfig eglconfig;
    EGLint neglconfigs;
    EGLContext eglctx;
    EGLBoolean ok;
    EGLint swapinterval = 1;
    unsigned int output_id = mir_display_output_id_invalid;
    char const* mir_socket = NULL;
    char const* dims = NULL;
    char const* cursor_name = mir_default_cursor_name;
    unsigned int rgb_bits = 8;
    mir_eglapp_bool help = 0, fullscreen = 0, no_vsync = 0, quiet = 0;

    const struct mir_eglapp_arg default_args[] =
    {
        {"-a", "<name>", "=", &appname, "Set application name"},
        {"-b", "<0.0-0.1>", "%f", &mir_eglapp_background_opacity, "Background opacity"},
        {"-c", "<name>", "=", &cursor_name, "Request cursor image by name"},
        {"-e", "<nbits>", "%u", &rgb_bits, "EGL colour channel size"},
        {"-f", "", "", &fullscreen, "Force full screen"},
        {"-h", "", "", &help, "Show this help text"},
        {"-m", "<socket>", "=", &mir_socket, "Mir server socket"},
        {"-n", "", "", &no_vsync, "Don't sync to vblank"},
        {"-o", "<id>", "%u", &output_id, "Force placement on output monitor ID"},
        {"-q", "", "", &quiet, "Quiet mode (no messages output)"},
        {"-s", "<width>x<height>", "=", &dims, "Force surface size"},
        {"--", "", "", NULL, "Ignore all arguments that follow"},
        {NULL, NULL, NULL, NULL, NULL}
    };

    const struct mir_eglapp_arg* const arg_lists[] =
    {
        /* TODO: custom args */
        default_args,
        NULL
    };

    if (!parse_args(argc, argv, arg_lists))
        return 0;

    if (help)
    {
        printf("Usage: %s [<options>]\n", argv[0]);
        show_help(arg_lists);
        return 0;
    }

    if (no_vsync)
        swapinterval = 0;

    if (dims && (2 != sscanf(dims, "%ux%u", width, height)))
    {
        fprintf(stderr, "Invalid dimensions: %s\n", dims);
        return 0;
    }

    if (quiet)
    {
        FILE *unused = freopen("/dev/null", "a", stdout);
        (void)unused;
    }

    connection = mir_connect_sync(mir_socket, appname);
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    egldisplay = eglGetDisplay(
                    mir_connection_get_egl_native_display(connection));
    CHECK(egldisplay != EGL_NO_DISPLAY, "Can't eglGetDisplay");

    ok = eglInitialize(egldisplay, NULL, NULL);
    CHECK(ok, "Can't eglInitialize");

    EGLint alpha_bits = mir_eglapp_background_opacity == 1.0f ? 0 : rgb_bits;
    const EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, rgb_bits,
        EGL_GREEN_SIZE, rgb_bits,
        EGL_BLUE_SIZE, rgb_bits,
        EGL_ALPHA_SIZE, alpha_bits,
        EGL_NONE
    };

    ok = eglChooseConfig(egldisplay, attribs, &eglconfig, 1, &neglconfigs);
    CHECK(ok, "Could not eglChooseConfig");
    CHECK(neglconfigs > 0, "No EGL config available");

    MirPixelFormat pixel_format =
        mir_connection_get_egl_pixel_format(connection, egldisplay, eglconfig);

    printf("Mir chose pixel format %d.\n", pixel_format);
    if (alpha_bits == 0)
    {
        /*
         * If we are opaque then it's OK to switch pixel format slightly,
         * to enable bypass/overlays to work. Otherwise the presence of an
         * alpha channel would prevent them from being used.
         * It would be really nice if Mesa just gave us the right answer in
         * the first place though. (LP: #1480755)
         */
        if (pixel_format == mir_pixel_format_abgr_8888)
            pixel_format = mir_pixel_format_xbgr_8888;
        else if (pixel_format == mir_pixel_format_argb_8888)
            pixel_format = mir_pixel_format_xrgb_8888;
    }
    printf("Using pixel format %d.\n", pixel_format);

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

    printf("Current active output is %dx%d %+d%+d\n",
           mode->horizontal_resolution, mode->vertical_resolution,
           output->position_x, output->position_y);

    if (fullscreen)  /* TODO: Use surface states for this */
    {
        *width = mode->horizontal_resolution;
        *height = mode->vertical_resolution;
    }

    mir_display_config_destroy(display_config);

    MirSurfaceSpec *spec =
        mir_connection_create_spec_for_normal_surface(connection, *width, *height, pixel_format);

    CHECK(spec != NULL, "Can't create a surface spec");

    char const* name = argv[0];
    for (char const* p = name; *p; p++)
    {
        if (*p == '/')
            name = p + 1;
    }
    mir_surface_spec_set_name(spec, name);

    if (output_id != mir_display_output_id_invalid)
        mir_surface_spec_set_fullscreen_on_output(spec, output_id);

    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    CHECK(mir_surface_is_valid(surface), "Can't create a surface");

    mir_surface_set_event_handler(surface, mir_eglapp_handle_event, NULL);
    
    MirCursorConfiguration *conf = mir_cursor_configuration_from_name(cursor_name);
    mir_surface_configure_cursor(surface, conf);
    mir_cursor_configuration_destroy(conf);

    eglsurface = eglCreateWindowSurface(egldisplay, eglconfig,
        (EGLNativeWindowType)mir_buffer_stream_get_egl_native_window(mir_surface_get_buffer_stream(surface)), NULL);
    
    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT,
                              ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    printf("Surface %d DPI\n", mir_surface_get_dpi(surface));
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
