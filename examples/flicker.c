/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <time.h>

static char const *socket_file = NULL;

static void render_pattern(MirGraphicsRegion *region, uint32_t pf)
{
    char *row = region->vaddr;
    int j;

    for (j = 0; j < region->height; j++)
    {
        int i;
        uint32_t *pixel = (uint32_t*)row;

        for (i = 0; i < region->width; i++)
        {
            pixel[i] = pf;
        }

        row += region->stride;
    }
}

static MirPixelFormat find_8888_format(MirPixelFormat *formats, unsigned int num_formats)
{
    MirPixelFormat pf = mir_pixel_format_invalid;

    for (unsigned int i = 0; i < num_formats; ++i)
    {
        MirPixelFormat cur_pf = formats[i];
        if (cur_pf == mir_pixel_format_abgr_8888 ||
            cur_pf == mir_pixel_format_xbgr_8888 ||
            cur_pf == mir_pixel_format_argb_8888 ||
            cur_pf == mir_pixel_format_xrgb_8888)
        {
            pf = cur_pf;
            break;
        }
    }

    assert(pf != mir_pixel_format_invalid);
    return pf;
}

static void fill_pattern(uint32_t pattern[2], MirPixelFormat pf)
{
    switch(pf)
    {
    case mir_pixel_format_abgr_8888:
    case mir_pixel_format_xbgr_8888:
        pattern[0] = 0xFF00FF00;
        pattern[1] = 0xFFFF0000;
        break;

    case mir_pixel_format_argb_8888:
    case mir_pixel_format_xrgb_8888:
        pattern[0] = 0xFF00FF00;
        pattern[1] = 0xFF0000FF;
        break;

    default:
        assert(0 && "Invalid pixel format");
    };
}

static volatile sig_atomic_t running = 1;

static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

static void handle_event(MirWindow* window, MirEvent const* ev, void* context_)
{
    MirRenderSurface* surface = (MirRenderSurface*)context_;

    switch (mir_event_get_type(ev))
    {
    case mir_event_type_resize:
    {
        MirResizeEvent const* resize = mir_event_get_resize_event(ev);
        int const new_width = mir_resize_event_get_width(resize);
        int const new_height = mir_resize_event_get_height(resize);

        mir_render_surface_set_size(surface, new_width, new_height);
        MirWindowSpec* spec = mir_create_window_spec(mir_window_get_connection(window));
        mir_window_spec_add_render_surface(spec, surface, new_width, new_height, 0, 0);
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


int main(int argc, char* argv[])
{
    static int const width = 640;
    static int const height = 480;

    MirWindow *window = 0;
    int swapinterval = 1;

    int arg;
    opterr = 0;
    while ((arg = getopt (argc, argv, "qhnm:")) != -1)
    {
        switch (arg)
        {
        case 'm':
            socket_file = optarg;
            break;
        case 'n':
            swapinterval = 0;
            break;
        case 'q':
            {
                FILE *unused = freopen("/dev/null", "a", stdout);
                (void)unused;
                break;
            }
        case '?':
        case 'h':
        default:
            printf("Usage: %s [<options>]\n"
                   "    -m <Mir server socket>  Connect to a specific Mir socket\n"
                   "    -h  Show this help text\n"
                   "    -n  Don't sync to vblank\n"
                   "    -q  Quiet mode (no messages output)\n"
                   , argv[0]);
            return -1;
        }
    }

    puts("Starting");

    MirConnection *const connection = mir_connect_sync(socket_file, __FILE__);
    assert(connection != NULL);
    assert(mir_connection_is_valid(connection));
    assert(strcmp(mir_connection_get_error_message(connection), "") == 0);
    puts("Connected");

    unsigned int const num_formats = 32;
    MirPixelFormat pixel_formats[num_formats];
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection, pixel_formats, num_formats, &valid_formats);
    MirPixelFormat pixel_format = find_8888_format(pixel_formats, valid_formats);

    MirRenderSurface *const surface = mir_connection_create_render_surface_sync(connection, width, height);
    puts("Surface created");

    MirWindowSpec *spec = mir_create_normal_window_spec(connection, width, height);
    assert(spec != NULL);
    mir_window_spec_set_name(spec, __FILE__);
    mir_window_spec_add_render_surface(spec, surface, width, height, 0, 0);
    mir_window_spec_set_event_handler(spec, handle_event, surface);

    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    assert(window != NULL);
    assert(mir_window_is_valid(window));
    assert(strcmp(mir_window_get_error_message(window), "") == 0);
    puts("Window created");

    MirBufferStream* bs = mir_render_surface_get_buffer_stream(surface, width, height, pixel_format);
    mir_buffer_stream_set_swapinterval(bs, swapinterval);

    uint32_t pattern[2] = {0};
    fill_pattern(pattern, pixel_format);

    MirGraphicsRegion graphics_region;
    int i=0;

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    while (running)
    {
        mir_buffer_stream_get_graphics_region(bs, &graphics_region);
        i++;
        render_pattern(&graphics_region, pattern[i & 1]);
        mir_buffer_stream_swap_buffers_sync(bs);
    }

    mir_window_release_sync(window);
    puts("Window released");

    mir_render_surface_release(surface);
    puts("Surface released");

    mir_connection_release(connection);
    puts("Connection released");

    return 0;
}
