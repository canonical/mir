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

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_tps.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

///\internal [MirDemoState_tag]
// Utility structure for the state of a single surface session.
typedef struct MirDemoState
{
    MirConnection *connection;
    MirSurface *surface;
} MirDemoState;
///\internal [MirDemoState_tag]

///\internal [Callback_tag]
// Callback to update MirDemoState on connection
static void connection_callback(MirConnection *new_connection, void *context)
{
    ((MirDemoState*)context)->connection = new_connection;
}

// Callback to update MirDemoState on surface_create
static void surface_create_callback(MirSurface *new_surface, void *context)
{
    ((MirDemoState*)context)->surface = new_surface;
}

// Callback to update MirDemoState on surface_release
static void surface_release_callback(MirSurface *old_surface, void *context)
{
    (void)old_surface;
    ((MirDemoState*)context)->surface = 0;
}
///\internal [Callback_tag]

void start_session(const char* server, const char* name, MirDemoState* mcd)
{
    ///\internal [connect_tag]
    // Call mir_connect and wait for callback to complete.
    mir_wait_for(mir_connect(server, name, connection_callback, mcd));
    puts("Connected for 'demo_client_app1'");
    ///\internal [connect_tag]

    // We expect a connection handle;
    // we expect it to be valid; and,
    // we don't expect an error description
    assert(mcd->connection != NULL);
    assert(mir_connection_is_valid(mcd->connection));
    assert(strcmp(mir_connection_get_error_message(mcd->connection), "") == 0);

    // We can query information about the platform we're running on
    {
        MirPlatformPackage platform_package;
        platform_package.data_items = -1;
        platform_package.fd_items = -1;

        mir_connection_get_platform(mcd->connection, &platform_package);
        assert(0 <= platform_package.data_items);
        assert(0 <= platform_package.fd_items);
    }
}

void stop_session(MirDemoState* mcd)
{
    if (mcd->surface)
    {
        ///\internal [surface_release_tag]
        // We should release our surface
        mir_wait_for(mir_surface_release(mcd->surface, surface_release_callback, mcd));
        puts("Surface released for 'demo_client_app1'");
        ///\internal [surface_release_tag]
    }

    ///\internal [connection_release_tag]
    // We should release our connection
    mir_connection_release(mcd->connection);
    puts("Connection released for 'demo_client_app1'");
    ///\internal [connection_release_tag]
}

void demo_client_app1(const char* server, pid_t trusted_helper_pid)
{
    MirDemoState mcd;
    mcd.connection = 0;
    mcd.surface = 0;
    start_session(server, "demo_client_trusted_session_app1", &mcd);

    // Identify a supported pixel format
    MirPixelFormat pixel_format;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(mcd.connection, &pixel_format, 1, &valid_formats);
    MirSurfaceParameters const request_params =
        {__PRETTY_FUNCTION__, 640, 480, pixel_format,
         mir_buffer_usage_hardware, mir_display_output_id_invalid};

    ///\internal [surface_create_tag]
    // ...we create a surface using that format and wait for callback to complete.
    mir_wait_for(mir_connection_create_surface(mcd.connection, &request_params, surface_create_callback, &mcd));
    puts("Surface created for 'demo_client_app1'");
    ///\internal [surface_create_tag]

    int status;
    printf("Waiting on trusted helper: %d: ", trusted_helper_pid);
    waitpid(trusted_helper_pid, &status, 0);

    // We expect a surface handle;
    // we expect it to be valid; and,
    // we don't expect an error description
    assert(mcd.surface != NULL);
    assert(mir_surface_is_valid(mcd.surface));
    assert(strcmp(mir_surface_get_error_message(mcd.surface), "") == 0);

    stop_session(&mcd);
}

// The main() function deals with parsing arguments and defaults
int main(int argc, char* argv[])
{
    // Some variables for holding command line options
    char const *server = NULL;

    // Parse the command line
    {
        int arg;
        opterr = 0;
        while ((arg = getopt (argc, argv, "c:hm:")) != -1)
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
                puts("Usage:");
                puts("    -m <Mir server socket>");
                puts("    -h: this help text");
                return -1;
            }
        }
    }

    const pid_t host_pid = getpid();
    pid_t pid = fork();

    if (pid == 0)
    {
        char buffer [15];
        sprintf(buffer, "%d", host_pid);
        char *array[] = {"mir_demo_client_trusted_session_trusted_helper", "-m", "server", "-p", buffer, NULL};

        errno = 0;
        int status = execvp("mir_demo_client_trusted_session_trusted_helper", array);
        printf("mir_demo_client_trusted_session_trusted_helper: status:%d - errno:%d\n", status, errno);
    }
    else if (pid > 0)
    {
        printf("mir_demo_client_trusted_session_app1: %d\n", getpid());
        demo_client_app1(server, pid);
    }

    return 0;
}
