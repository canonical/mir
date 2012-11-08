/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir_client/mir_client_library.h"

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

static char const *socket_file = "/tmp/mir_socket";
static int buffer_swap_count = 0;
static MirConnection *connection = 0;
static MirSurface *surface = 0;

static void set_connection(MirConnection *new_connection, void * context)
{
    (void)context;
    connection = new_connection;
}

static void surface_create_callback(MirSurface *new_surface, void *context)
{
    (void)context;
    surface = new_surface;
}

static void surface_next_buffer_callback(MirSurface* surface, void *context)
{
    (void) surface;
    (void) context;
}

static void surface_release_callback(MirSurface *old_surface, void *context)
{
    (void)old_surface;
    (void)context;
    surface = 0;
}

int main(int argc, char* argv[])
{
    int arg;
    opterr = 0;
    while ((arg = getopt (argc, argv, "c:hf:")) != -1)
    {
        switch (arg)
        {
            case 'c':
                buffer_swap_count = atoi(optarg);
                break;
            case 'f':
                socket_file = optarg;
                break;
                
            case '?':
            case 'h':
            default:
                puts(argv[0]);
                puts("Usage:");
                puts("    -f <socket filename>");
                puts("    -h: this help text");
                return -1;
        }
    }

    puts("Starting");

    mir_wait_for(mir_connect(socket_file, __PRETTY_FUNCTION__, set_connection, 0));
    puts("Connected");

    assert(connection != NULL);
    assert(mir_connection_is_valid(connection));
    assert(strcmp(mir_connection_get_error_message(connection), "") == 0);

    MirPlatformPackage platform_package;
    platform_package.data_items = -1;
    platform_package.fd_items = -1;

    mir_connection_get_platform(connection, &platform_package);
    assert(0 <= platform_package.data_items);
    assert(0 <= platform_package.fd_items);


    MirSurfaceParameters const request_params =
        {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888, mir_buffer_usage_hardware};
    mir_wait_for(mir_surface_create(connection, &request_params, surface_create_callback, 0));
    puts("Surface created");

    assert(surface != NULL);
    assert(mir_surface_is_valid(surface));
    assert(strcmp(mir_surface_get_error_message(surface), "") == 0);

    MirSurfaceParameters response_params;
    mir_surface_get_parameters(surface, &response_params);
    assert(request_params.width == response_params.width);
    assert(request_params.height ==  response_params.height);
    assert(request_params.pixel_format == response_params.pixel_format);

    MirBufferPackage buffer_package;
    buffer_package.data_items = -1;
    buffer_package.fd_items = -1;
    mir_surface_get_current_buffer(surface, &buffer_package);
    assert(0 <= buffer_package.data_items);
    assert(0 <= buffer_package.fd_items);

    int i;
    for(i = 0; i < buffer_swap_count; i++)
    {
        mir_wait_for(mir_surface_next_buffer(surface, surface_next_buffer_callback, 0));
    }
    
    mir_wait_for(mir_surface_release(surface, surface_release_callback, 0));
    puts("Surface released");

    mir_connection_release(connection);
    puts("Connection released");

    return 0;
}



