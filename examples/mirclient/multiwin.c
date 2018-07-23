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
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>

typedef struct
{
    uint8_t r, g, b, a;
} Color;

typedef struct
{
    MirConnection *conn;
    MirRenderSurface* surface;
    MirWindow *window;
    MirBufferStream* bs;
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

static void premultiply_alpha(Color *c)
{
    c->r = (unsigned)c->r * c->a / 255U;
    c->g = (unsigned)c->g * c->a / 255U;
    c->b = (unsigned)c->b * c->a / 255U;
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
            /* Not filling in the X byte is correct but buggy (LP: #1423462) */
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
            /* Not filling in the X byte is correct but buggy (LP: #1423462) */
            (uint32_t)color->r << 16 |
            (uint32_t)color->g << 8  |
            (uint32_t)color->b;
        break;
    case mir_pixel_format_rgb_888:
        for (n = 0; n < count; n++)
        {
            uint8_t *p = (uint8_t*)where + n * 3;
            p[0] = color->r;
            p[1] = color->g;
            p[2] = color->b;
        }
        count = 0;
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
    case mir_pixel_format_rgb_565:
        for (n = 0; n < count; n++)
        {
            uint16_t *p = (uint16_t*)where + n;
            *p = (uint16_t)(color->r >> 3) << 11 |
                 (uint16_t)(color->g >> 2) << 5  |
                 (uint16_t)(color->b >> 3);
        }
        count = 0;
        break;
    case mir_pixel_format_rgba_5551:
        for (n = 0; n < count; n++)
        {
            uint16_t *p = (uint16_t*)where + n;
            *p = (uint16_t)(color->r >> 3) << 11 |
                 (uint16_t)(color->g >> 3) << 6  |
                 (uint16_t)(color->b >> 3) << 1  |
                 (uint16_t)(color->a ? 1 : 0);
        }
        count = 0;
        break;
    case mir_pixel_format_rgba_4444:
        for (n = 0; n < count; n++)
        {
            uint16_t *p = (uint16_t*)where + n;
            *p = (uint16_t)(color->r >> 4) << 12 |
                 (uint16_t)(color->g >> 4) << 8  |
                 (uint16_t)(color->b >> 4) << 4  |
                 (uint16_t)(color->a >> 4);
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
    mir_buffer_stream_get_graphics_region(win->bs, &region);
    clear_region(&region, &win->fill);
    mir_buffer_stream_swap_buffers_sync(win->bs);
}

static char const *socket_file = NULL;

static void handle_event(MirWindow* window, MirEvent const* ev, void* context_)
{
    Window* context = (Window*)context_;

    switch (mir_event_get_type(ev))
    {
    case mir_event_type_resize:
    {
        MirResizeEvent const* resize = mir_event_get_resize_event(ev);
        int const new_width = mir_resize_event_get_width(resize);
        int const new_height = mir_resize_event_get_height(resize);

        mir_render_surface_set_size(context->surface, new_width, new_height);
        MirWindowSpec* spec = mir_create_window_spec(context->conn);
        mir_window_spec_add_render_surface(spec, context->surface, new_width, new_height, 0, 0);
        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
        break;
    }

    case mir_event_type_close_window:
        running = 0;
        printf("Received close event from server.\n");
        break;

    default:
        break;
    }
}

static void init_window(MirConnection *conn, Window* win, char const* name, int width, int height, Color color, MirPixelFormat pixel_format)
{
    win->conn = conn;
    win->surface = mir_connection_create_render_surface_sync(conn, width, height);
    win->bs = mir_render_surface_get_buffer_stream(win->surface, width, height, pixel_format);
    win->fill = color;
    premultiply_alpha(&win->fill);

    MirWindowSpec *spec= mir_create_normal_window_spec(conn, width, height);
    mir_window_spec_set_name(spec, name);
    mir_window_spec_add_render_surface(spec, win->surface, width, height, 0, 0);
    mir_window_spec_set_event_handler(spec, &handle_event, win);
    win->window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
}

int main(int argc, char *argv[])
{
    MirConnection *conn;
    Window win[3];
    unsigned int f;
    MirPixelFormat pixel_format = mir_pixel_format_invalid;
    int alpha = 0x50;

    int arg;
    opterr = 0;
    while ((arg = getopt (argc, argv, "hm:a:p:")) != -1)
    {
        switch (arg)
        {
        case 'a':
            alpha = atoi(optarg);
            break;

        case 'p':
            pixel_format = (MirPixelFormat)atoi(optarg);
            break;

        case 'm':
            socket_file = optarg;
            break;

        case '?':
        case 'h':
        default:
            puts(argv[0]);
            puts("Usage:");
            puts("    -m <Mir server socket>");
            puts("    -a      Alpha for surfaces");
            puts("    -p <N>  Force pixel format N");
            puts("    -h      This help text");
            return -1;
        }
    }

    conn = mir_connect_sync(socket_file, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server: %s\n", mir_connection_get_error_message(conn));
        return 1;
    }

    if (pixel_format == mir_pixel_format_invalid)
    {
        MirPixelFormat formats[mir_pixel_formats];
        unsigned int valid_formats;
        mir_connection_get_available_surface_formats(conn, formats, mir_pixel_formats, &valid_formats);

        for (f = 0; f < valid_formats; f++)
        {
            if (formats[f] == mir_pixel_format_abgr_8888 ||
                formats[f] == mir_pixel_format_argb_8888)
            {
                pixel_format = formats[f];
                break;
            }
        }
        if (pixel_format == mir_pixel_format_invalid)
        {
            fprintf(stderr, "Could not find a fast 32-bit pixel format with "
                            "alpha support. Blending won't work!.\n");
            pixel_format = formats[0];
        }
    }

    Color red = {0xff, 0x00, 0x00, alpha};
    Color green = {0x00, 0xff, 0x00, alpha};
    Color blue = {0x00, 0x00, 0xff, alpha};

    init_window(conn, win+0, "red",   255, 255, red,   pixel_format);
    init_window(conn, win+1, "green", 300, 150, green, pixel_format);
    init_window(conn, win+2, "blue",  150, 300, blue,  pixel_format);

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    while (running)
    {
        for (int w = 0; w < (int)(sizeof(win)/sizeof(win[0])); w++)
            draw_window(win + w);
    }

    for (int w = 0; w < (int)(sizeof(win)/sizeof(win[0])); w++)
    {
        mir_window_release_sync(win[w].window);
        mir_render_surface_release(win[w].surface);
    }

    mir_connection_release(conn);

    return 0;
}
