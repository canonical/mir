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

#define _GNU_SOURCE // for nanosleep

#include "eglapp.h"

#include "mir_toolkit/mir_client_library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <GLES2/gl2.h>

void animate_cursor(MirBufferStream *stream)
{
    static double alpha = 0.0;
    char fill_color = 0xff * alpha;
    
    MirGraphicsRegion region;
    mir_buffer_stream_get_graphics_region(stream, &region);
    
    memset(region.vaddr, fill_color, region.stride*region.height);
    
    mir_buffer_stream_swap_buffers_sync(stream);
    alpha += 0.01;
    if (alpha >= 1.0)
        alpha = 0.0;
    
}

MirBufferStream* make_cursor_stream(MirConnection *connection, MirSurface *surface)
{
    MirBufferStream* stream = mir_connection_create_buffer_stream_sync(connection,
      24, 24, mir_pixel_format_argb_8888, mir_buffer_usage_software);

    animate_cursor(stream);

    MirCursorConfiguration* conf = mir_cursor_configuration_from_buffer_stream(stream, 0, 0);
    mir_wait_for(mir_surface_configure_cursor(surface, conf));
    mir_cursor_configuration_destroy(conf);
    
    return stream;
}

int main(int argc, char *argv[])
{
    unsigned int width = 128, height = 128;

    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    glClearColor(0.5, 0.5, 0.5, mir_eglapp_background_opacity);
    glClear(GL_COLOR_BUFFER_BIT);
    mir_eglapp_swap_buffers();

    MirBufferStream* stream = make_cursor_stream(mir_eglapp_native_connection(),
        mir_eglapp_native_surface());

    struct timespec onehundred_millis = {
        0, 100*1000000
    };

    while (mir_eglapp_running())
    {
        nanosleep(&onehundred_millis, &onehundred_millis);
        animate_cursor(stream);
    }

    mir_eglapp_shutdown();

    return 0;
}
