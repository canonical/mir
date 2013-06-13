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

#include "mir_toolkit/mir_client_library.h"
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>  /* sleep() */
#include <string.h>

#define BYTES_PER_PIXEL(f) ((f) == mir_pixel_format_bgr_888 ? 3 : 4)
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

typedef struct
{
    uint8_t r, g, b, a;
} Color;

static volatile sig_atomic_t running = 1;

static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

static void put_pixels(void *where, int count, MirPixelFormat format,
                       const Color *color)
{
    uint32_t pixel = 0;
    int n;

    switch (format)
    {
    case mir_pixel_format_abgr_8888:
        pixel = 
            (uint32_t)color->a << 24 |
            (uint32_t)color->b << 16 |
            (uint32_t)color->g << 8  |
            (uint32_t)color->r;
        break;
    case mir_pixel_format_xbgr_8888:
        pixel = 
            (uint32_t)color->b << 16 |
            (uint32_t)color->g << 8  |
            (uint32_t)color->r;
        break;
    case mir_pixel_format_argb_8888:
        pixel = 
            (uint32_t)color->a << 24 |
            (uint32_t)color->r << 16 |
            (uint32_t)color->g << 8  |
            (uint32_t)color->b;
        break;
    case mir_pixel_format_xrgb_8888:
        pixel = 
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

    for (n = 0; n < count; n++)
        ((uint32_t*)where)[n] = pixel;
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

static void redraw(MirSurface *surface, const MirGraphicsRegion *canvas)
{
    MirGraphicsRegion backbuffer;

    mir_surface_get_graphics_region(surface, &backbuffer);
    copy_region(&backbuffer, canvas);
    mir_surface_swap_buffers_sync(surface);
}

static void on_event(MirSurface *surface, const MirEvent *event, void *context)
{
    MirGraphicsRegion *canvas = (MirGraphicsRegion*)context;
    (void)surface;

    static const Color color[] =
    {
        {0x00, 0xff, 0xff, 0xff},
        {0xff, 0x00, 0xff, 0xff},
        {0x00, 0xff, 0x00, 0xff},
        {0xff, 0xff, 0x00, 0xff},
        {0x00, 0x00, 0xff, 0xff},
        {0xff, 0x00, 0x00, 0xff},
        {0xff, 0xff, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x80, 0x80, 0xff},
        {0x80, 0x00, 0x80, 0xff},
        {0x00, 0x80, 0x00, 0xff},
        {0x80, 0x80, 0x00, 0xff},
        {0x00, 0x00, 0x80, 0xff},
        {0x80, 0x00, 0x00, 0xff},
        {0x80, 0x80, 0x80, 0xff},
        {0x00, 0x00, 0x00, 0xff},
    };

    if (event->type == mir_event_type_motion)
    {
        static size_t base_color = 0;
        static size_t max_fingers = 0;

        if (event->motion.action == mir_motion_action_up)
        {
            base_color = (base_color + max_fingers) %
                         (sizeof(color)/sizeof(color[0]));
            max_fingers = 0;
        }

        if (event->motion.action == mir_motion_action_move)
        {
            size_t p;

            if (event->motion.pointer_count > max_fingers)
                max_fingers = event->motion.pointer_count;

            for (p = 0; p < event->motion.pointer_count; p++)
            {
                int x = event->motion.pointer_coordinates[p].x;
                int y = event->motion.pointer_coordinates[p].y;
                int radius = event->motion.pointer_coordinates[p].pressure *
                             event->motion.pointer_coordinates[p].size * 50.0f;
                size_t c = (base_color + p) %
                           (sizeof(color)/sizeof(color[0]));
                draw_box(canvas, x - radius, y - radius, 2*radius, color + c);
            }
    
            redraw(surface, canvas);
        }
    }
}

int main(int argc, char *argv[])
{
    static const Color background = {0x40, 0x40, 0x40, 0xff};
    MirConnection *conn;
    MirSurfaceParameters parm;
    MirDisplayInfo dinfo;
    MirSurface *surf;
    MirGraphicsRegion canvas;
    MirEventDelegate delegate = {&on_event, &canvas};
    int f;

    (void)argc;

    conn = mir_connect_sync(NULL, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server.\n");
        return 1;
    }

    mir_connection_get_display_info(conn, &dinfo);

    parm.buffer_usage = mir_buffer_usage_software;
    parm.pixel_format = mir_pixel_format_invalid;
    for (f = 0; f < dinfo.supported_pixel_format_items; f++)
    {
        if (dinfo.supported_pixel_format[f] == mir_pixel_format_abgr_8888 ||
            dinfo.supported_pixel_format[f] == mir_pixel_format_argb_8888)
        {
            parm.pixel_format = dinfo.supported_pixel_format[f];
            break;
        }
    }
    if (parm.pixel_format == mir_pixel_format_invalid)
    {
        fprintf(stderr, "Could not find a fast 32-bit pixel format with "
                        "alpha support. Blending won't work!.\n");
        parm.pixel_format = dinfo.supported_pixel_format[0];
    }

    parm.name = "Paint Canvas";
    parm.width = dinfo.width;
    parm.height = dinfo.height;
    surf = mir_connection_create_surface_sync(conn, &parm);

    mir_surface_set_event_handler(surf, &delegate);

    canvas.width = parm.width;
    canvas.height = parm.height;
    canvas.stride = canvas.width * BYTES_PER_PIXEL(parm.pixel_format);
    canvas.pixel_format = parm.pixel_format;
    canvas.vaddr = (char*)malloc(canvas.stride * canvas.height);

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    clear_region(&canvas, &background);
    redraw(surf, &canvas);

    while (running)
    {
        sleep(1);  /* Is there a better way yet? */
    }

    mir_connection_release(conn);

    return 0;
}
