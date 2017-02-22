/*
 * Copyright © 2015 Canonical LTD
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
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#include "eglapp.h"

#include "mir_toolkit/mir_client_library.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <GLES2/gl2.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirRenderSurface* render_surface;
#pragma GCC diagnostic pop

void animate_cursor(MirBufferStream *stream)
{
    // mir_pixel_format_argb_8888 as set below
    int const bpp = 4;
    uint32_t const background = 0x00000000;
    uint32_t const foreground = 0xffffffff;

    MirGraphicsRegion region;
    mir_buffer_stream_get_graphics_region(stream, &region);
    
    for (int y = 0; y < region.height; ++y)
    {
        for (int x = 0; x < region.width; ++x)
        {
            uint32_t* pixel = (uint32_t*)
                              (region.vaddr + y * region.stride + x * bpp);
            *pixel = background;
        }
    }

    static float theta = 0.0f;
    theta += 0.000234567f;

    char* origin = region.vaddr + (region.height/2)*region.stride;

    for (int x = 0; x < region.width; ++x)
    {
        int const magnitude = region.height / 3;
        int y = magnitude * sinf(theta + x * 2 * M_PI / region.width);
        uint32_t* pixel = (uint32_t*)(origin + x*bpp + y*region.stride);
        *pixel = foreground;
    }
    
    mir_buffer_stream_swap_buffers_sync(stream);
}

MirBufferStream* make_cursor_surface(MirConnection *connection, MirWindow *surface)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    render_surface =
        mir_connection_create_render_surface_sync(connection, 24, 24);

    MirBufferStream* stream =
        mir_render_surface_get_buffer_stream(render_surface, 24, 24, mir_pixel_format_argb_8888);

    animate_cursor(stream);

    MirCursorConfiguration* conf =
        mir_cursor_configuration_from_render_surface(render_surface, 0, 0);
#pragma GCC diagnostic pop

    mir_window_configure_cursor(surface, conf);
    mir_cursor_configuration_destroy(conf);
    
    return stream;
}

int main(int argc, char *argv[])
{
    unsigned int width = 128, height = 128;

    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
        return 1;

    glClearColor(0.5, 0.5, 0.5, mir_eglapp_background_opacity);
    glClear(GL_COLOR_BUFFER_BIT);
    mir_eglapp_swap_buffers();

    MirBufferStream* stream = make_cursor_surface(mir_eglapp_native_connection(),
        mir_eglapp_native_window());

    while (mir_eglapp_running())
    {
        animate_cursor(stream);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_render_surface_release(render_surface);
#pragma GCC diagnostic pop

    mir_eglapp_cleanup();

    return 0;
}
