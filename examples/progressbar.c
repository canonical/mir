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

#define _DEFAULT_SOURCE
#define _BSD_SOURCE /* for usleep() */

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

static const Color blue = {0, 0, 255, 255};
static const Color white = {255, 255, 255, 255};

static const Color *const foreground = &white;
static const Color *const background = &blue;

static volatile sig_atomic_t running = 1;

static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
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

    if (alpha_shift >= 0 && color->a < 255)
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

static void redraw(MirSurface *surface, const MirGraphicsRegion *canvas)
{
    MirGraphicsRegion backbuffer;
    MirBufferStream *bs = mir_surface_get_buffer_stream(surface);

    mir_buffer_stream_get_graphics_region(bs, &backbuffer);
    clear_region(&backbuffer, background);
    copy_region(&backbuffer, canvas);
    mir_buffer_stream_swap_buffers_sync(bs);
}

int main(int argc, char *argv[])
{
    MirConnection *conn;
    MirSurface *surf;
    MirGraphicsRegion canvas;
    unsigned int f;
    unsigned int const pf_size = 32;
    MirPixelFormat formats[pf_size];
    unsigned int valid_formats;
    int hz = 20;

    if (argc > 1)
    {
        int rate;

        if (sscanf(argv[1], "%d", &rate) == 1 && rate > 0)
        {
            hz = rate;
        }
        else
        {
            fprintf(stderr, "Usage: %s [repeat rate in Hz]\n"
                            "Default repeat rate is %d\n",
                    argv[0], hz);

            return 1;
        }
    }

    conn = mir_connect_sync(NULL, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server: %s\n", mir_connection_get_error_message(conn));
        return 1;
    }

    mir_connection_get_available_surface_formats(conn, formats, pf_size,
        &valid_formats);

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

    int width = 500;
    int height = 500;
    MirSurfaceSpec *spec =
        mir_connection_create_spec_for_normal_surface(conn, width, height, pixel_format);
    if (spec == NULL)
    {
        fprintf(stderr, "Could not create a surface spec.\n");
        mir_connection_release(conn);
        return 1;
    }

    {
        char name[128];
        snprintf(name, sizeof(name)-1, "Progress Bars (%dHz)", hz);
        name[sizeof(name)-1] = '\0';
        mir_surface_spec_set_name(spec, name);
    }

    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_software);

    surf = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    if (surf != NULL)
    {
        canvas.width = width;
        canvas.height = height;
        canvas.stride = canvas.width * BYTES_PER_PIXEL(pixel_format);
        canvas.pixel_format = pixel_format;
        canvas.vaddr = (char*)malloc(canvas.stride * canvas.height);

        if (canvas.vaddr != NULL)
        {
            int t = 0;

            signal(SIGINT, shutdown);
            signal(SIGTERM, shutdown);
            signal(SIGHUP, shutdown);
        
            while (running)
            {
                static const int box_width = 8;
                static const int space = 1;
                const int grid = box_width + 2 * space;
                const int row = width / grid;
                const int square = row * row;
                const int x = (t % row) * grid + space;
                const int y = (t / row) * grid + space;

                if (t % square == 0)
                    clear_region(&canvas, background);

                t = (t + 1) % square;

                draw_box(&canvas, x, y, box_width, foreground);

                redraw(surf, &canvas);
                usleep(1000000 / hz);
            }

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
