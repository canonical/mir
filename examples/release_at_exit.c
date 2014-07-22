/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static MirConnection *connection = 0;
static MirSurface *surface = 0;

static void connection_callback(MirConnection *new_connection, void *context)
{
    (void)context;
    connection = new_connection;
}

static void surface_create_callback(MirSurface *new_surface, void *context)
{
    (void)context;
    surface = new_surface;
}

static void surface_release_callback(MirSurface *old_surface, void *context)
{
    (void)old_surface;
    (void)context;
    surface = 0;
}

static void close_connection()
{
    if (connection)
    {
        mir_connection_release(connection);
    }
}


void demo_client(const char* server)
{
    atexit(&close_connection);

    mir_wait_for(mir_connect(server, __FILE__, connection_callback, NULL));
    assert(mir_connection_is_valid(connection));

    MirPixelFormat pixel_format;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);
    MirSurfaceParameters const request_params =
        {NULL, 640, 480, pixel_format,
         mir_buffer_usage_hardware, mir_display_output_id_invalid};

    mir_wait_for(mir_connection_create_surface(connection, &request_params, surface_create_callback, NULL));
    mir_wait_for(mir_surface_release(surface, surface_release_callback, NULL));
}

int main(int argc, char* argv[])
{
    // Some variables for holding command line options
    char const *server = NULL;

    // Parse the command line
    {
        int arg;
        opterr = 0;
        while ((arg = getopt (argc, argv, "hm:")) != -1)
        {
            switch (arg)
            {
            case 'm':
                server = optarg;
                break;

            case '?':
            case 'h':
            default:
                puts(argv[0]);
                puts("Cutdown version of mir_demo_client_basic to show that");
                puts("clients can release the connection after main() exits.");
                puts("Usage:");
                puts("    -m <Mir server socket>");
                puts("    -h: this help text");
                return -1;
            }
        }
    }

    demo_client(server);
    return 0;
}
