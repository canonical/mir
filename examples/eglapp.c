/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include <errno.h>
#include <string.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <xkbcommon/xkbcommon-keysyms.h>

float mir_eglapp_background_opacity = 1.0f;

static const char* appname = "egldemo";

static MirConnection *connection;
static MirRenderSurface* surface;
static MirWindow* window;
static EGLDisplay egldisplay;
static EGLSurface eglsurface;
static volatile sig_atomic_t running = 0;
static double refresh_rate = 0.0;
static mir_eglapp_bool alt_vsync = 0;

#define CHECK(_cond, _err) \
    if (!(_cond)) \
    { \
        printf("%s\n", (_err)); \
        return 0; \
    }

void mir_eglapp_cleanup(void)
{
    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(egldisplay);
    if (surface)
    {
        mir_render_surface_release(surface);
        surface = NULL;
    }
    mir_window_release_sync(window);
    window = NULL;
    mir_connection_release(connection);
    connection = NULL;
}

void mir_eglapp_quit(void)
{
    running = 0;
}

static void shutdown(int signum)
{
    if (running)
    {
        mir_eglapp_quit();
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

static void mir_eglapp_handle_window_event(MirWindowEvent const* sev)
{
    MirWindowAttrib attrib = mir_window_event_get_attribute(sev);
    int value = mir_window_event_get_attribute_value(sev);

    switch (attrib)
    {
    case mir_window_attrib_visibility:
        printf("Window %s\n", value == mir_window_visibility_exposed ?
                               "exposed" : "occluded");
        break;
    case mir_window_attrib_dpi:
        // value is still zero - never implemented. Deprecate? (LP: #1559831)
        break;
    default:
        break;
    }
}

static void handle_window_output_event(MirWindowOutputEvent const* out)
{
    static char const* const form_factor_name[6] =
        {"unknown", "phone", "tablet", "monitor", "TV", "projector"};
    unsigned ff = mir_window_output_event_get_form_factor(out);
    char const* form_factor = (ff < 6) ? form_factor_name[ff] : "out-of-range";

    refresh_rate = mir_window_output_event_get_refresh_rate(out);

    printf("Window is on output %u: %d DPI, scale %.1fx, %s form factor, %.2fHz\n",
           mir_window_output_event_get_output_id(out),
           mir_window_output_event_get_dpi(out),
           mir_window_output_event_get_scale(out),
           form_factor,
           refresh_rate);
}

double mir_eglapp_display_hz(void)
{
    return refresh_rate;
}

void mir_eglapp_handle_event(MirWindow* window, MirEvent const* ev, void* unused)
{
    (void) unused;

    switch (mir_event_get_type(ev))
    {
    case mir_event_type_input:
        mir_eglapp_handle_input_event(mir_event_get_input_event(ev));
        break;
    case mir_event_type_window:
        mir_eglapp_handle_window_event(mir_event_get_window_event(ev));
        break;
    case mir_event_type_window_output:
        handle_window_output_event(mir_event_get_window_output_event(ev));
        break;
    case mir_event_type_resize:
        /*
         * FIXME: https://bugs.launchpad.net/mir/+bug/1194384
         * It is unsafe to set the width and height here because we're in a
         * different thread to that doing the rendering. So we either need
         * support for event queuing (directing them to another thread) or
         * full single-threaded callbacks. (LP: #1194384).
         */
        egl_app_handle_resize_event(window, mir_event_get_resize_event(ev));
        break;
    case mir_event_type_close_window:
        printf("Received close event from server.\n");
        running = 0;
        break;
    default:
        break;
    }
}

void egl_app_handle_resize_event(MirWindow* window, MirResizeEvent const* resize)
{
    int const new_width = mir_resize_event_get_width(resize);
    int const new_height = mir_resize_event_get_height(resize);

    printf("Resized to %dx%d\n", new_width, new_height);
    if (surface)
    {
        mir_render_surface_set_size(surface, new_width, new_height);
        MirWindowSpec* spec = mir_create_window_spec(connection);
        mir_window_spec_add_render_surface(spec, surface, new_width, new_height, 0, 0);
        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
    }
}

static void show_help(struct mir_eglapp_arg const* const* arg_lists)
{
    int const indent = 2, desc_offset = 2;
    struct mir_eglapp_arg const* const* list;
    int max_len = 0;

    for (list = arg_lists; *list != NULL; ++list)
    {
        struct mir_eglapp_arg const* arg;
        for (arg = *list; arg->syntax != NULL; ++arg)
        {
            int len = indent + strlen(arg->syntax);
            if (len > max_len)
                max_len = len;
        }
    }
    for (list = arg_lists; *list != NULL; ++list)
    {
        struct mir_eglapp_arg const* arg;
        for (arg = *list; arg->syntax != NULL; ++arg)
        {
            int len = 0, remainder = 0;
            printf("%*c%s%n", indent, ' ', arg->syntax, &len);
            remainder = desc_offset + max_len - len;
            printf("%*c%s", remainder, ' ', arg->description);
            switch (arg->format[0])
            {
            case '=':
                {
                    char const* str = *(char const**)arg->variable;
                    if (str)
                        printf(" [%s]", str);
                }
                break;
            case '%':
                switch (arg->format[1])
                {
                case 'u': printf(" [%u]", *(unsigned*)arg->variable); break;
                case 'f': printf(" [%.1f]", *(float*)arg->variable); break;
                default: break;
                }
            default:
                break;
            }
            printf("\n");
        }
    }
}

static mir_eglapp_bool parse_args(int argc, char *argv[],
                             struct mir_eglapp_arg const* const* arg_lists)
{
    for (int i = 1; i < argc; ++i)
    {
        char const* operator = argv[i];
        struct mir_eglapp_arg const* const* list;
        for (list = arg_lists; *list != NULL; ++list)
        {
            struct mir_eglapp_arg const* arg;
            for (arg = *list; arg->syntax != NULL; ++arg)
            {
                char const* space = strchr(arg->syntax, ' ');
                int operator_len = space != NULL ? space - arg->syntax
                                                 : (int)strlen(arg->syntax);
                if (!strncmp(operator, arg->syntax, operator_len))
                {
                    char const* operand = operator + operator_len;
                    if (!operand[0] && strchr("=%", arg->format[0]))
                    {
                        if (i+1 < argc)
                            operand = argv[++i];
                        else
                        {
                            fprintf(stderr, "Missing argument for %s\n",
                                    operator);
                            return 0;
                        }
                    }
                    switch (arg->format[0])
                    {
                    case '$': return 1;  /* -- is special */
                    case '!': *(mir_eglapp_bool*)arg->variable = 1; break;
                    case '=': *(char const**)arg->variable = operand; break;
                    case '%':
                        if (!sscanf(operand, arg->format, arg->variable))
                        {
                            fprintf(stderr,
                                    "Invalid option: %s  (expected %s)\n",
                                    operand, arg->syntax);
                            return 0;
                        }
                        break;
                    default: abort(); break;
                    }
                    goto next;
                }
            }
        }
        fprintf(stderr, "Unknown option: %s\n", operator);
        return 0;
        next:
        (void)0; /* Stop compiler warnings about label at the end */
    }
    return 1;
}

mir_eglapp_bool mir_eglapp_init(int argc, char* argv[],
                                unsigned int* width, unsigned int* height,
                                struct mir_eglapp_arg const* custom_args)
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
    mir_eglapp_bool help = 0, no_vsync = 0, quiet = 0;
    mir_eglapp_bool fullscreen = !*width || !*height;

    struct mir_eglapp_arg const default_args[] =
    {
        {"-a <name>", "=", &appname, "Set application name"},
        {"-b <0.0-0.1>", "%f", &mir_eglapp_background_opacity, "Background opacity"},
        {"-c <name>", "=", &cursor_name, "Request cursor image by name"},
        {"-e <nbits>", "%u", &rgb_bits, "EGL colour channel size"},
        {"-f", "!", &fullscreen, "Force full screen"},
        {"-h", "!", &help, "Show this help text"},
        {"-m <socket>", "=", &mir_socket, "Mir server socket"},
        {"-n", "!", &no_vsync, "Don't sync to vblank"},
        {"-v", "!", &alt_vsync, "Sync to vblank using the alternate method (manual timing)"},
        {"-o <id>", "%u", &output_id, "Force placement on output monitor ID"},
        {"-q", "!", &quiet, "Quiet mode (no messages output)"},
        {"-s <width>x<height>", "=", &dims, "Force window size"},
        {"--", "$", NULL, "Ignore all arguments that follow"},
        {NULL, NULL, NULL, NULL}
    };

    struct mir_eglapp_arg const* const arg_lists[] =
    {
        default_args,
        custom_args,
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

    if (no_vsync || alt_vsync)
        swapinterval = 0;

    if (dims)
    {
        if (2 != sscanf(dims, "%ux%u", width, height))
        {
            fprintf(stderr, "Invalid dimensions: %s\n", dims);
            return 0;
        }
        fullscreen = 0;
    }

    if (quiet)
    {
        FILE *unused = freopen("/dev/null", "a", stdout);
        (void)unused;
    }

    connection = mir_connect_sync(mir_socket, appname);
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    egldisplay = eglGetDisplay(connection);

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

    MirWindowSpec *spec =
        mir_create_normal_window_spec(connection, *width, *height);

    CHECK(spec != NULL, "Can't create a window spec");

    if (fullscreen)
    {
        mir_window_spec_set_state(spec, mir_window_state_fullscreen);

        MirDisplayConfig* display_config =
            mir_connection_create_display_configuration(connection);

        int const count = mir_display_config_get_num_outputs(display_config);

        for (int i = 0; i != count; ++i)
        {
            MirOutput const* output = mir_display_config_get_output(display_config, i);

            if (mir_output_get_connection_state(output) == mir_output_connection_state_connected &&
                mir_output_is_enabled(output))
            {
                MirOutputMode const* mode = mir_output_get_current_mode(output);
                *width = mir_output_mode_get_width(mode);
                *height = mir_output_mode_get_height(mode);

                break;
            }
        }

        mir_display_config_release(display_config);
    }

    surface = mir_connection_create_render_surface_sync(connection, *width, *height);
    CHECK(mir_render_surface_is_valid(surface), "could not create surface");
    CHECK(mir_render_surface_get_error_message(surface), "");
    mir_window_spec_add_render_surface(spec, surface, *width, *height, 0, 0);

    char const* name = argv[0];
    for (char const* p = name; *p; p++)
    {
        if (*p == '/')
            name = p + 1;
    }
    mir_window_spec_set_name(spec, name);
    mir_window_spec_set_event_handler(spec, mir_eglapp_handle_event, NULL);
    mir_window_spec_set_cursor_name(spec, cursor_name);

    if (output_id != mir_display_output_id_invalid)
        mir_window_spec_set_fullscreen_on_output(spec, output_id);

    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    CHECK(mir_window_is_valid(window), "Can't create a window");

    eglsurface = eglCreateWindowSurface(egldisplay,
                                        eglconfig,
                                        (EGLNativeWindowType)surface,
                                        NULL);

    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT,
                              ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    EGLint buf_width, buf_height;
    if (eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &buf_width) &&
        eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &buf_height))
    {
        /*
         * Mir reserves the right to ignore our initial window dimensions and
         * resize to whatever it likes. In that case, a resize callback to
         * mir_eglapp_handle_event() has occurred by now and we have resized
         * the eglsurface.
         */
        *width = buf_width;
        *height = buf_height;
    }

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    eglSwapInterval(egldisplay, swapinterval);

    running = 1;

    return 1;
}

struct MirConnection* mir_eglapp_native_connection()
{
    return connection;
}

MirWindow* mir_eglapp_native_window()
{
    return window;
}
