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

#define _POSIX_C_SOURCE 199309L  /* to get struct timespec */
#include "mir_toolkit/mir_client_library.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

static volatile sig_atomic_t running = 1;

static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

static long long now()  /* in nanoseconds */
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}

int main(int argc, char *argv[])
{
    const char *server = NULL;
    int interval = 1;

    for (int a = 1; a < argc; a++)
    {
        const char *arg = argv[a];
        if (arg[0] == '-')
        {
            switch (arg[1])
            {
                case 'f':
                    interval = 0;
                    break;
                case 'h':
                default:
                    printf("Usage: %s [-f] [-h] [<socket-file>]\n"
                           "Options:\n"
                           "    -f  Flood (no delay). On multi-core systems this can eliminate context\n"
                           "        switching delays, at the expense of occupying all the CPU time.\n"
                           "    -h  Show this help information.\n"
                           , argv[0]);
                    return 0;
            }
        }
        else
        {
            server = arg;
        }
    }

    MirConnection *conn = mir_connect_sync(server, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server.\n");
        return 1;
    }

    MirPixelFormat formats[1];
    unsigned int valid_formats = 0;
    mir_connection_get_available_surface_formats(conn, formats, 1,
                                                 &valid_formats);

    MirSurfaceParameters parm;
    parm.buffer_usage = mir_buffer_usage_software;
    parm.output_id = mir_display_output_id_invalid;
    parm.pixel_format = formats[0];
    parm.name = "ping";
    parm.width = 1;
    parm.height = 1;

    MirSurface *surf = mir_connection_create_surface_sync(conn, &parm);
    if (surf == NULL || !mir_surface_is_valid(surf))
    {
        fprintf(stderr, "Could not create a surface.\n");
        mir_connection_release(conn);
        return 2;
    }

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    static const MirSurfaceType types[2] =
    {
        mir_surface_type_normal,
        mir_surface_type_overlay
    };
    int t = 1;

    printf("Connected to server: %s\n", server == NULL ? "<default>" : server);
    while (running)
    {
        long long start, duration;

        start = now();
        mir_wait_for(mir_surface_set_type(surf, types[t]));
        duration = now() - start;
        t ^= 1;

        printf("Round-trip time: %ld.%06ld milliseconds\n",
               (long)(duration / 1000000),
               (long)(duration % 1000000));

        if (interval) sleep(interval);
    }

    mir_surface_release_sync(surf);
    mir_connection_release(conn);

    return 0;
}
