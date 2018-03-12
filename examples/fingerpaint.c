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

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/events/input/input_event.h"

#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>  /* sleep() */
#include <string.h>
#include <pthread.h>

#define BYTES_PER_PIXEL(f) ((f) == mir_pixel_format_bgr_888 ? 3 : 4)
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

typedef struct
{
    uint8_t r, g, b, a;
} Color;

static volatile bool running = true;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t change = PTHREAD_COND_INITIALIZER;
static bool changed = true;
static int force_radius = 0;

static void shutdown(int signum)
{
    if (running)
    {
        running = false;
        changed = true;
        pthread_cond_signal(&change);
        printf("Signal %d received. Good night.\n", signum);
    }
}

static void blend(uint32_t *dest, uint32_t src, int alpha_shift)
{
    uint8_t *d = (uint8_t*)dest;
    uint8_t *s = (uint8_t*)&src;
    uint32_t src_alpha = (uint32_t)(src >> alpha_shift) & 0xff;
    uint32_t dest_alpha = 0xff - src_alpha;
    int i;

    for (i = 0; i < 4; i++)
    {
        d[i] = (uint8_t)
               (
                   (
                       ((uint32_t)d[i] * dest_alpha) +
                       ((uint32_t)s[i] * src_alpha)
                   ) >> 8   /* Close enough, and faster than /255 */
               );
    }

    *dest |= (0xff << alpha_shift); /* Restore alpha 1.0 in the destination */
}

static void put_pixels(void *where, int count, MirPixelFormat format,
                       const Color *color)
{
    uint32_t pixel = 0;
    int alpha_shift = -1;
    int n;

    /*
     * We are blending in software, so can pretend that
     *   mir_pixel_format_abgr_8888 == mir_pixel_format_xbgr_8888
     *   mir_pixel_format_argb_8888 == mir_pixel_format_xrgb_8888
     */
    switch (format)
    {
    case mir_pixel_format_abgr_8888:
    case mir_pixel_format_xbgr_8888:
        alpha_shift = 24;
        pixel = 
            (uint32_t)color->a << 24 |
            (uint32_t)color->b << 16 |
            (uint32_t)color->g << 8  |
            (uint32_t)color->r;
        break;
    case mir_pixel_format_argb_8888:
    case mir_pixel_format_xrgb_8888:
        alpha_shift = 24;
        pixel = 
            (uint32_t)color->a << 24 |
            (uint32_t)color->r << 16 |
            (uint32_t)color->g << 8  |
            (uint32_t)color->b;
        break;
    case mir_pixel_format_bgr_888:
        for (n = 0; n < count; n++)
        {
            uint8_t *p = (uint8_t*)where + n * 3;
            p[0] = color->b;
            p[1] = color->g;
            p[2] = color->r;
        }
        count = 0;
        break;
    default:
        count = 0;
        break;
    }

    if (alpha_shift >= 0)
    {
        for (n = 0; n < count; n++)
            blend((uint32_t*)where + n, pixel, alpha_shift);
    }
    else
    {
        for (n = 0; n < count; n++)
            ((uint32_t*)where)[n] = pixel;
    }
}

static void clear_region(const MirGraphicsRegion *region, const Color *color)
{
    int y;
    char *row = region->vaddr;

    for (y = 0; y < region->height; y++)
    {
        put_pixels(row, region->width, region->pixel_format, color);
        row += region->stride;
    }
}

static void draw_box(const MirGraphicsRegion *region, int x, int y, int size,
                     const Color *color)
{
    if (x >= 0 && y >= 0 && x+size < region->width && y+size < region->height)
    {
        int j;
        char *row = region->vaddr +
                    (y * region->stride) +
                    (x * BYTES_PER_PIXEL(region->pixel_format));
    
        for (j = 0; j < size; j++)
        {
            put_pixels(row, size, region->pixel_format, color);
            row += region->stride;
        }
    }
}

static void copy_region(const MirGraphicsRegion *dest,
                        const MirGraphicsRegion *src)
{
    int height = MIN(src->height, dest->height);
    int width = MIN(src->width, dest->width);
    int y;
    const char *srcrow = src->vaddr;
    char *destrow = dest->vaddr;
    int copy = width * BYTES_PER_PIXEL(dest->pixel_format);

    for (y = 0; y < height; y++)
    {
        memcpy(destrow, srcrow, copy);
        srcrow += src->stride;
        destrow += dest->stride;
    }
}

typedef struct Context
{
    MirConnection *connection;
    MirRenderSurface* surface;
    MirWindow* window;
    MirGraphicsRegion* canvas;
} Context;

static void on_event(MirWindow *window, const MirEvent *event, void *context_)
{
    Context* context = (Context*)context_;

    pthread_mutex_lock(&mutex);

    switch (mir_event_get_type(event))
    {
    case mir_event_type_resize:
    {
        MirResizeEvent const* resize = mir_event_get_resize_event(event);
        int const new_width = mir_resize_event_get_width(resize);
        int const new_height = mir_resize_event_get_height(resize);

        mir_render_surface_set_size(context->surface, new_width, new_height);
        MirWindowSpec* spec = mir_create_window_spec(context->connection);
        mir_window_spec_add_render_surface(spec, context->surface, new_width, new_height, 0, 0);
        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
        break;
    }

    case mir_event_type_close_window:
    {
        static int closing = 0;

        ++closing;
        if (closing == 1)
            printf("Sure you don't want to save your work?\n");
        else if (closing > 1)
        {
            printf("Oh I forgot you can't save your work. Quitting now...\n");
            running = false;
            changed = true;
        }
        break;
    }

    case mir_event_type_input:
    {
        static const Color color[] =
            {
                {0x80, 0xff, 0x00, 0xff},
                {0x00, 0xff, 0x80, 0xff},
                {0xff, 0x00, 0x80, 0xff},
                {0xff, 0x80, 0x00, 0xff},
                {0x00, 0x80, 0xff, 0xff},
                {0x80, 0x00, 0xff, 0xff},
                {0xff, 0xff, 0x00, 0xff},
                {0x00, 0xff, 0xff, 0xff},
                {0xff, 0x00, 0xff, 0xff},
                {0xff, 0x00, 0x00, 0xff},
                {0x00, 0xff, 0x00, 0xff},
                {0x00, 0x00, 0xff, 0xff},
            };

        static size_t base_color = 0;
        static size_t max_fingers = 0;
        static float max_pressure = 1.0f;

        MirInputEvent const* input_event = mir_event_get_input_event(event);
        MirTouchEvent const* tev = NULL;
        MirPointerEvent const* pev = NULL;
        unsigned touch_count = 0;
        bool ended = false;
        MirInputEventType type = mir_input_event_get_type(input_event);

        switch (type)
        {
        case mir_input_event_type_touch:
            tev = mir_input_event_get_touch_event(input_event);
            touch_count = mir_touch_event_point_count(tev);
            ended = touch_count == 1 &&
                    (mir_touch_event_action(tev, 0) == mir_touch_action_up);
            break;
        case mir_input_event_type_pointer:
            pev = mir_input_event_get_pointer_event(input_event);
            ended = mir_pointer_event_action(pev) ==
                    mir_pointer_action_button_up;
            touch_count = mir_pointer_event_button_state(pev, mir_pointer_button_primary) ? 1 : 0;
        default:
            break;
        }

        if (ended)
        {
            base_color = (base_color + max_fingers) %
                         (sizeof(color)/sizeof(color[0]));
            max_fingers = 0;
        }
        else if (touch_count)
        {
            size_t p;

            if (touch_count > max_fingers)
                max_fingers = touch_count;

            for (p = 0; p < touch_count; p++)
            {
                int x = 0;
                int y = 0;
                int radius = 1;
                float pressure = 1.0f;

                if (tev != NULL)
                {
                    x = mir_touch_event_axis_value(tev, p, mir_touch_axis_x);
                    y = mir_touch_event_axis_value(tev, p, mir_touch_axis_y);
                    float m = mir_touch_event_axis_value(tev, p,
                                                         mir_touch_axis_touch_major);
                    float n = mir_touch_event_axis_value(tev, p,
                                                         mir_touch_axis_touch_minor);
                    radius = (m + n) / 4;  /* Half the average */
                    // mir_touch_axis_touch_major can be 0
                    if (radius < 5)
                        radius = 5;
                    pressure = mir_touch_event_axis_value(tev, p,
                                                          mir_touch_axis_pressure);
                }
                else if (pev != NULL)
                {
                    x = mir_pointer_event_axis_value(pev, mir_pointer_axis_x);
                    y = mir_pointer_event_axis_value(pev, mir_pointer_axis_y);
                    pressure = 0.5f;
                    radius = 5;
                }

                if (force_radius)
                    radius = force_radius;

                size_t c = (base_color + p) %
                           (sizeof(color)/sizeof(color[0]));
                Color tone = color[c];

                if (pressure > max_pressure)
                    max_pressure = pressure;
                pressure /= max_pressure;
                tone.a *= pressure;

                draw_box(context->canvas, x - radius, y - radius, 2*radius, &tone);
            }

            changed = true;
        }

        break;
    }

    default:
        break;
    }

    pthread_cond_signal(&change);
    pthread_mutex_unlock(&mutex);
}

static MirOutput const* find_active_output(
    MirDisplayConfig const* conf)
{
    size_t num_outputs = mir_display_config_get_num_outputs(conf);

    for (size_t i = 0; i < num_outputs; i++)
    {
        MirOutput const* output = mir_display_config_get_output(conf, i);
        MirOutputConnectionState state = mir_output_get_connection_state(output);
        if (state == mir_output_connection_state_connected && mir_output_is_enabled(output))
        {
            return output;
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    static const Color background = {180, 180, 150, 255};
    MirGraphicsRegion canvas;
    Context context = { NULL, NULL, NULL, &canvas };

    int swap_interval = 0;

    char *mir_socket = NULL;

    if (argc > 1)
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            int help = 0;
            const char *arg = argv[i];

            if (arg[0] == '-')
            {
                if (arg[1] == '-' && arg[2] == '\0')
                    break;

                switch (arg[1])
                {
                case 'm':
                    ++i;
                    if (i < argc)
                        mir_socket = argv[i];
                    else
                        help = 1;
                    break;
                case 'r':
                    ++i;
                    if (i < argc)
                        force_radius = atoi(argv[i]);
                    else
                        help = 1;
                    break;
                case 'w':
                    swap_interval = 1;
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
                       "  -h               Show this help text\n"
                       "  -m <socket path> Mir server socket\n"
                       "  -r <radius>      Force paint brush radius\n"
                       "  -w               Wait for vblank (don't drop frames)\n"
                       "  --               Ignore further arguments\n"
                       , argv[0]);
                return 0;
            }
        }
    }

    context.connection = mir_connect_sync(mir_socket, argv[0]);
    if (!mir_connection_is_valid(context.connection))
    {
        fprintf(stderr, "Could not connect to a display server: %s\n", mir_connection_get_error_message(context.connection));
        return 1;
    }

    MirDisplayConfig* display_config =
        mir_connection_create_display_configuration(context.connection);

    MirOutput const* output = find_active_output(display_config);
    if (output == NULL)
    {
        fprintf(stderr, "No active outputs found.\n");
        mir_connection_release(context.connection);
        return 1;
    }

    MirPixelFormat pixel_format = mir_pixel_format_invalid;
    size_t num_pfs = mir_output_get_num_pixel_formats(output);

    for (size_t i = 0; i < num_pfs; i++)
    {
        MirPixelFormat f = mir_output_get_pixel_format(output, i);
        if (BYTES_PER_PIXEL(f) == 4)
        {
            pixel_format = f;
            break;
        }
    }

    if (pixel_format == mir_pixel_format_invalid)
    {
        fprintf(stderr, "Could not find a fast 32-bit pixel format\n");
        mir_connection_release(context.connection);
        return 1;
    }

    MirOutputMode const* mode = mir_output_get_current_mode(output);
    int width  = mir_output_mode_get_width(mode);
    int height = mir_output_mode_get_height(mode);

    mir_display_config_release(display_config);

    context.surface = mir_connection_create_render_surface_sync(context.connection, width, height);
    MirWindowSpec *spec = mir_create_normal_window_spec(context.connection, width, height);
    mir_window_spec_set_name(spec, "Mir Fingerpaint");
    mir_window_spec_add_render_surface(spec, context.surface, width, height, 0, 0);

    context.window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    if (context.window != NULL)
    {
        mir_window_set_event_handler(context.window, &on_event, &context);
        MirBufferStream* bs = mir_render_surface_get_buffer_stream(context.surface, width, height, pixel_format);
        mir_buffer_stream_set_swapinterval(bs, swap_interval);

        canvas.width = width;
        canvas.height = height;
        canvas.stride = canvas.width * BYTES_PER_PIXEL(pixel_format);
        canvas.pixel_format = pixel_format;
        canvas.vaddr = (char*)malloc(canvas.stride * canvas.height);

        if (canvas.vaddr != NULL)
        {
            signal(SIGINT, shutdown);
            signal(SIGTERM, shutdown);
            signal(SIGHUP, shutdown);

            clear_region(&canvas, &background);

            while (running)
            {
                MirGraphicsRegion backbuffer;
                mir_buffer_stream_get_graphics_region(bs, &backbuffer);

                pthread_mutex_lock(&mutex);
                while (!changed)
                    pthread_cond_wait(&change, &mutex);
                changed = false;
                copy_region(&backbuffer, &canvas);
                pthread_mutex_unlock(&mutex);

                mir_buffer_stream_swap_buffers_sync(bs);
            }

            /* Ensure canvas won't be used after it's freed */
            mir_window_set_event_handler(context.window, NULL, NULL);
            free(canvas.vaddr);
        }
        else
        {
            fprintf(stderr, "Failed to malloc canvas\n");
        }

        mir_window_release_sync(context.window);
    }
    else
    {
        fprintf(stderr, "mir_connection_create_surface_sync failed\n");
    }

    mir_render_surface_release(context.surface);
    mir_connection_release(context.connection);

    return 0;
}
