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

#define NWIN 3

typedef struct
{
    unsigned char r, g, b, a;
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

static void clear_region(const MirGraphicsRegion *region, const Color *color)
{
    int y;
    unsigned char *row = (unsigned char *)region->vaddr;
    int pixelsize = region->pixel_format == mir_pixel_format_bgr_888 ? 3 : 4;

    for (y = 0; y < region->height; y++)
    {
        int x;
        unsigned char *pixel = row;

        for (x = 0; x < region->width; x++)
        {
            int c;
            for (c = 0; c < pixelsize; c++)
            {
                /* FIXME */
                pixel[0] = color->r;
                pixel[1] = color->g;
                pixel[2] = color->b;
                pixel[3] = 0xff;
            }
            pixel += pixelsize;
        }

        row += region->stride;
    }
}

static void draw_window(Window *win)
{
    MirGraphicsRegion region;

    mir_surface_get_graphics_region(win->surface, &region);
    clear_region(&region, &win->fill);
    mir_surface_next_buffer_sync(win->surface);
}

int main(int argc, char *argv[])
{
    MirConnection *conn;
    MirSurfaceParameters parm;
    MirDisplayInfo dinfo;
    Window win[NWIN];

    (void)argc;

    conn = mir_connect_sync(NULL, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server.\n");
        return 1;
    }

    mir_connection_get_display_info(conn, &dinfo);

    parm.name = "foo";
    parm.pixel_format = dinfo.supported_pixel_format[0];
    parm.buffer_usage = mir_buffer_usage_software;

    parm.width = 200;
    parm.height = 200;
    win[0].surface = mir_connection_create_surface_sync(conn, &parm);
    win[0].fill.r = 0xff;
    win[0].fill.g = 0x00;
    win[0].fill.b = 0x00;

    parm.width = 300;
    parm.height = 150;
    win[1].surface = mir_connection_create_surface_sync(conn, &parm);
    win[1].fill.r = 0x00;
    win[1].fill.g = 0xff;
    win[1].fill.b = 0x00;

    parm.width = 150;
    parm.height = 300;
    win[2].surface = mir_connection_create_surface_sync(conn, &parm);
    win[2].fill.r = 0x00;
    win[2].fill.g = 0x00;
    win[2].fill.b = 0xff;

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    while (running)
    {
        int i;
        for (i = 0; i < NWIN; i++)
            draw_window(win + i);
    }

    mir_connection_release(conn);

    return 0;
}
