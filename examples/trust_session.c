/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind <nick.dedekind@canonical.com>
 */

#define _POSIX_SOURCE

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_trust_session.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

///\page trust_session.c trust_session.c: A mir client which starts a trust session and trusted client app.
/// mir_demo_client_trust_session shows the use of mir trust session API.
/// This program opens a mir connection and creates a trust session.
///\section demo_client_trusted_helper demo_client_trusted_helper()
/// Opens a mir connection and creates a trust session
/// before closing the trust session and connection.
///\section demo_client_trust_session_app demo_client_trust_session_app()
/// Opens a mir connection and creates a surface
/// before releasing the surface and closing the connection.
///\example trust_session.c A mir client demonstrating trust sessions.
///\section MirDemoState MirDemoState
/// The handles needs to be accessible both to callbacks and to the control function.
///\snippet trust_session.c MirDemoState_tag
///\section Callbacks Callbacks
///\snippet trust_session.c Callback_tag
/// This program opens a mir connection and starts a trust session.

///\internal [MirDemoState_tag]
// Utility structure for the state of a single session.
typedef struct MirDemoState
{
    MirConnection *connection;
    MirSurface *surface;
    MirTrustSession *trust_session;
    pid_t  child_pid;
} MirDemoState;
///\internal [MirDemoState_tag]


///\internal [Callback_tag]
// Callback to update MirDemoState on trust_session_event
static void trust_session_event_callback(MirTrustSession* trust_session, MirTrustSessionState state, void* context)
{
    (void)trust_session;
    MirDemoState* demo_state = (MirDemoState*)context;

    printf("Trust Session state updated to %d\n", state);
    if (state == mir_trust_session_stopped)
    {
        kill(demo_state->child_pid, SIGINT);
    }
}
///\internal [Callback_tag]

void start_session(const char* server, const char* name, MirDemoState* mcd)
{
    // Call mir_connect synchronously
    mcd->connection = mir_connect_sync(server, name);

    // We expect a connection handle;
    // we expect it to be valid; and,
    // we don't expect an error description
    assert(mcd->connection != NULL);
    assert(mir_connection_is_valid(mcd->connection));
    assert(strcmp(mir_connection_get_error_message(mcd->connection), "") == 0);
    printf("Connected for '%s'\n", name);

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
    if (mcd->trust_session)
    {
        mir_trust_session_release(mcd->trust_session);
        mcd->trust_session = 0;
        puts("Trust session released for 'demo_client_trusted_helper'");
    }

    if (mcd->surface)
    {
        // We should release our surface
        mir_surface_release_sync(mcd->surface);
        mcd->surface = 0;
        puts("Surface released for 'demo_client_app2'");
    }

    // We should release our connection
    mir_connection_release(mcd->connection);
    puts("Connection released for 'demo_client_trusted_helper'");
}

void demo_client_trusted_helper(const char* server, pid_t child_pid)
{
    MirDemoState mcd;
    mcd.connection = 0;
    mcd.surface = 0;
    mcd.trust_session = 0;
    mcd.child_pid = child_pid;
    start_session(server, "demo_client_trust_session_trusted_helper", &mcd);

    // We create a trust session
    mcd.trust_session = mir_trust_session_create(mcd.connection);
    assert(mcd.trust_session != NULL);
    MirTrustSessionAddApplicationResult add_result = mir_trust_session_add_app_with_pid(mcd.trust_session, child_pid);
    assert(add_result == mir_trust_session_app_addition_succeeded);

    mir_trust_session_start_sync(mcd.trust_session);
    assert(mir_trust_session_is_started(mcd.trust_session) == mir_true);
    puts("Started Trust Helper for 'demo_client_trusted_helper'");

    // Case where the session is stopped by server.
    mir_trust_session_set_event_callback(mcd.trust_session, trust_session_event_callback, &mcd);

    int status;
    printf("Waiting on child app: %d\n", child_pid);
    waitpid(child_pid, &status, 0);

    if (mir_trust_session_is_started(mcd.trust_session) == mir_true)
    {
        mir_trust_session_stop_sync(mcd.trust_session);
        assert(mir_trust_session_is_started(mcd.trust_session) == mir_false);
        puts("Stopped Trust Helper for 'demo_client_trusted_helper'");
    }

    stop_session(&mcd);
}

void demo_client_trust_session_app(const char* server)
{
    MirDemoState mcd;
    mcd.connection = 0;
    mcd.surface = 0;
    mcd.trust_session = 0;
    start_session(server, "demo_client_trust_session_app", &mcd);

    // Identify a supported pixel format
    MirPixelFormat pixel_format;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(mcd.connection, &pixel_format, 1, &valid_formats);
    MirSurfaceParameters const request_params =
        {__PRETTY_FUNCTION__, 640, 480, pixel_format,
         mir_buffer_usage_hardware, mir_display_output_id_invalid};

    // ...we create a surface using that format and wait for callback to complete.
    mcd.surface = mir_connection_create_surface_sync(mcd.connection, &request_params);

    // We expect a surface handle;
    // we expect it to be valid; and,
    // we don't expect an error description
    assert(mcd.surface != NULL);
    assert(mir_surface_is_valid(mcd.surface));
    assert(strcmp(mir_surface_get_error_message(mcd.surface), "") == 0);
    puts("Surface created for 'demo_client_trust_session_app'");

    // Wait for stdin
    char buff[1];
    read(STDIN_FILENO, &buff, 1);
    puts("demo_client_trust_session_app Done");

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

    pid_t pid = fork();

    if (pid == 0)
    {
        demo_client_trust_session_app(server);
    }
    else if (pid > 0)
    {
        printf("demo_client_trusted_helper: this:%d , child:%d\n", getpid(), pid);
        demo_client_trusted_helper(server, pid);
    }

    return 0;
}
