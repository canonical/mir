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

#define _POSIX_C_SOURCE 200112L  // for setenv() from stdlib.h

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

static void on_event(MirSurface *surface, const MirEvent *event, void *context)
{
    (void)surface;
    MirGraphicsRegion *canvas = (MirGraphicsRegion*)context;

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
    
    MirEventType event_type = mir_event_get_type(event);

    pthread_mutex_lock(&mutex);

    if (event_type == mir_event_type_input)
    {
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
            touch_count = mir_pointer_event_button_state(pev,
                               mir_pointer_button_primary) ? 1 : 0;
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
                    float size = mir_touch_event_axis_value(tev, p,
                                                          mir_touch_axis_size);
                    pressure = mir_touch_event_axis_value(tev, p,
                                                      mir_touch_axis_pressure);
                    radius = size * 50.0f + 1.0f;
                }
                else if (pev != NULL)
                {
                    x = mir_pointer_event_axis_value(pev, mir_pointer_axis_x);
                    y = mir_pointer_event_axis_value(pev, mir_pointer_axis_y);
                    pressure = 0.5f;
                    radius = 5;
                }

                size_t c = (base_color + p) %
                           (sizeof(color)/sizeof(color[0]));
                Color tone = color[c];

                if (pressure > max_pressure)
                    max_pressure = pressure;
                pressure /= max_pressure;
                tone.a *= pressure;

                draw_box(canvas, x - radius, y - radius, 2*radius, &tone);
            }
    
            changed = true;
        }
    }
    else if (event_type == mir_event_type_close_surface)
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
    }
    else if (event_type == mir_event_type_resize)
    {
        changed = true;
    }

    pthread_cond_signal(&change);
    pthread_mutex_unlock(&mutex);
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

int main(int argc, char *argv[])
{
    static const Color background = {180, 180, 150, 255};
    MirConnection *conn;
    MirSurface *surf;
    MirGraphicsRegion canvas;
    unsigned int f;
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
                switch (arg[1])
                {
                case 'm':
                    mir_socket = argv[++i];
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
                       "  -m socket        Mir server socket\n"
                       "  -w               Wait for vblank (don't drop frames)\n"
                       , argv[0]);
                return 0;
            }
        }
    }

    // We do our own resampling now. We can keep up with raw input...
    // TODO: Replace setenv with a proper Mir function (LP: #1439590)
    setenv("MIR_CLIENT_INPUT_RATE", "0", 0);

    conn = mir_connect_sync(mir_socket, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server: %s\n", mir_connection_get_error_message(conn));
        return 1;
    }

    MirDisplayConfiguration *display_config =
        mir_connection_create_display_config(conn);

    const MirDisplayOutput *dinfo = find_active_output(display_config);
    if (dinfo == NULL)
    {
        fprintf(stderr, "No active outputs found.\n");
        mir_connection_release(conn);
        return 1;
    }

    unsigned int const pf_size = 32;
    MirPixelFormat formats[pf_size];
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(conn, formats, pf_size, &valid_formats);

    MirPixelFormat pixel_format = mir_pixel_format_invalid;
    for (f = 0; f < valid_formats; f++)
    {
        if (BYTES_PER_PIXEL(formats[f]) == 4)
        {
            pixel_format = formats[f];
            break;
        }
    }

    if (pixel_format == mir_pixel_format_invalid)
    {
        fprintf(stderr, "Could not find a fast 32-bit pixel format\n");
        mir_connection_release(conn);
        return 1;
    }


    int width = dinfo->modes[dinfo->current_mode].horizontal_resolution;
    int height = dinfo->modes[dinfo->current_mode].vertical_resolution;

    mir_display_config_destroy(display_config);

    MirSurfaceSpec *spec = mir_connection_create_spec_for_normal_surface(conn, width, height, pixel_format);
    mir_surface_spec_set_name(spec, "Mir Fingerpaint");
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_software);

    surf = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    if (surf != NULL)
    {
        mir_surface_set_swapinterval(surf, swap_interval);
        mir_surface_set_event_handler(surf, &on_event, &canvas);
    
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
        
            MirBufferStream *bs = mir_surface_get_buffer_stream(surf);

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
            mir_surface_set_event_handler(surf, NULL, NULL);
            free(canvas.vaddr);
        }
        else
        {
            fprintf(stderr, "Failed to malloc canvas\n");
        }

        mir_surface_release_sync(surf);
    }
    else
    {
        fprintf(stderr, "mir_connection_create_surface_sync failed\n");
    }

    mir_connection_release(conn);

    return 0;
}
