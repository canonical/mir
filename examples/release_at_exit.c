/*
 * Copyright © 2012-2014 Canonical Ltd.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static MirConnection *connection = 0;
static MirWindow* window = 0;

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

    connection = mir_connect_sync(server, __FILE__);
    assert(mir_connection_is_valid(connection));

    MirPixelFormat pixel_format;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);
    MirWindowSpec *spec = mir_create_normal_window_spec(connection, 640, 480);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    mir_window_release_sync(window);
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
