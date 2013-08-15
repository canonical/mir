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

typedef struct
{
    uint8_t r, g, b, a;
} Color;

typedef struct
{
    MirSurface *surface;
    Color fill;
} Window;

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

static void draw_window(Window *win)
{
    MirGraphicsRegion region;

    mir_surface_get_graphics_region(win->surface, &region);
    clear_region(&region, &win->fill);
    mir_surface_swap_buffers_sync(win->surface);
}

int main(int argc, char *argv[])
{
    MirConnection *conn;
    MirSurfaceParameters parm;
    Window win[3];
    unsigned int f;

    (void)argc;

    conn = mir_connect_sync(NULL, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server.\n");
        return 1;
    }

    unsigned int const pf_size = 32;
    MirPixelFormat formats[pf_size];
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(conn, formats, pf_size, &valid_formats);

    parm.buffer_usage = mir_buffer_usage_software;
    parm.pixel_format = mir_pixel_format_invalid;
    for (f = 0; f < valid_formats; f++)
    {
        if (formats[f] == mir_pixel_format_abgr_8888 ||
            formats[f] == mir_pixel_format_argb_8888)
        {
            parm.pixel_format = formats[f];
            break;
        }
    }
    if (parm.pixel_format == mir_pixel_format_invalid)
    {
        fprintf(stderr, "Could not find a fast 32-bit pixel format with "
                        "alpha support. Blending won't work!.\n");
        parm.pixel_format = formats[0];
    }

    parm.name = "red";
    parm.width = 225;
    parm.height = 225;
    win[0].surface = mir_connection_create_surface_sync(conn, &parm);
    win[0].fill.r = 0xff;
    win[0].fill.g = 0x00;
    win[0].fill.b = 0x00;
    win[0].fill.a = 0x50;

    parm.name = "green";
    parm.width = 300;
    parm.height = 150;
    win[1].surface = mir_connection_create_surface_sync(conn, &parm);
    win[1].fill.r = 0x00;
    win[1].fill.g = 0xff;
    win[1].fill.b = 0x00;
    win[1].fill.a = 0x50;

    parm.name = "blue";
    parm.width = 150;
    parm.height = 300;
    win[2].surface = mir_connection_create_surface_sync(conn, &parm);
    win[2].fill.r = 0x00;
    win[2].fill.g = 0x00;
    win[2].fill.b = 0xff;
    win[2].fill.a = 0x50;

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    while (running)
    {
        int w;
        for (w = 0; w < (int)(sizeof(win)/sizeof(win[0])); w++)
            draw_window(win + w);
    }

    mir_surface_release_sync(win[0].surface);
    mir_surface_release_sync(win[1].surface);
    mir_surface_release_sync(win[2].surface);
    mir_connection_release(conn);

    return 0;
}
