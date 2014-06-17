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
#include "mir_toolkit/mir_prompt_session.h"

#undef NDEBUG
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

///\page prompt_session.c prompt_session.c: A mir client which starts a prompt session and prompt client app.
/// mir_demo_client_prompt_session shows the use of mir prompt session API.
/// This program opens a mir connection and creates a prompt session.
///\section helper helper()
/// Opens a mir connection and creates a prompt session
/// before closing the prompt session and connection.
///\section prompt_session_app prompt_session_app()
/// Opens a mir connection and creates a surface
/// before releasing the surface and closing the connection.
///\example prompt_session.c A mir client demonstrating prompt sessions.
///\section MirDemoState MirDemoState
/// The handles needs to be accessible both to callbacks and to the control function.
///\snippet prompt_session.c MirDemoState_tag
///\section Callbacks Callbacks
///\snippet prompt_session.c Callback_tag
/// This program creates two processes, both opening a mir connection, one starting
/// a prompt session with the other process.

///\internal [MirDemoState_tag]
// Utility structure for the state of a single session.
typedef struct MirDemoState
{
    MirConnection *connection;
    MirSurface *surface;
    MirPromptSession *prompt_session;
    pid_t  child_pid;
    MirPromptSessionState state;

    int* client_fds;
    unsigned int client_fd_count;
} MirDemoState;
///\internal [MirDemoState_tag]


///\internal [Callback_tag]
// Callback to update MirDemoState on prompt_session_event
static void prompt_session_event_callback(MirPromptSession* prompt_session,
                                         MirPromptSessionState state,
                                         void* context)
{
    (void)prompt_session;
    MirDemoState* demo_state = (MirDemoState*)context;
    demo_state->state = state;

    printf("helper: Prompt Session state updated to %d\n", state);
    if (state == mir_prompt_session_state_stopped)
    {
        kill(demo_state->child_pid, SIGINT);
    }
}

static void client_fd_callback(MirPromptSession* prompt_session, size_t count, int const* fds, void* context)
{
    (void)prompt_session;
    ((MirDemoState*)context)->client_fds = malloc(sizeof(int)*count);
    unsigned int i = 0;
    for (; i < count; i++)
    {
        ((MirDemoState*)context)->client_fds[i] = fds[i];
    }
    ((MirDemoState*)context)->client_fd_count = count;
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
    printf("%s: Connected\n", name);

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

void stop_session(MirDemoState* mcd, const char* name)
{
    if (mcd->surface)
    {
        // We should release our surface
        mir_surface_release_sync(mcd->surface);
        mcd->surface = 0;
        printf("%s: Surface released\n", name);
    }

    // We should release our connection
    mir_connection_release(mcd->connection);
    printf("%s: Connection released\n", name);
}

void helper(const char* server)
{
    MirDemoState mcd;
    mcd.connection = 0;
    mcd.surface = 0;
    mcd.prompt_session = 0;
    mcd.state = mir_prompt_session_state_stopped;
    mcd.client_fd_count = 0;
    start_session(server, "helper", &mcd);

    // We create a prompt session
    mcd.prompt_session = mir_connection_create_prompt_session_sync(mcd.connection, getpid(), prompt_session_event_callback, &mcd);
    assert(mcd.prompt_session != NULL);

    assert(mcd.state == mir_prompt_session_state_started);
    puts("helper: Started prompt session");

    mir_wait_for(mir_prompt_session_new_fds_for_prompt_providers(mcd.prompt_session, 1, client_fd_callback, &mcd));
    assert(mcd.client_fd_count == 1);
    puts("helper: Added waiting FD");

    printf("helper: Starting child application 'mir_demo_client_basic' with fd://%d\n", mcd.client_fds[0]);
    mcd.child_pid = fork();

    if (mcd.child_pid == 0)
    {
        char buffer[128] = {0};
        sprintf(buffer, "fd://%d", mcd.client_fds[0]);

        char* args[4];
        args[0] = "mir_demo_client_basic";
        args[1] = "-m";
        args[2] = &buffer[0];
        args[3] = NULL;

        errno = 0;
        execvp("mir_demo_client_basic", args);
        return;
    }

    int status;
    printf("helper: Waiting on child application: %d\n", mcd.child_pid);
    waitpid(mcd.child_pid, &status, 0);

    if (mcd.state == mir_prompt_session_state_started)
    {
        mir_prompt_session_release_sync(mcd.prompt_session);
        mcd.prompt_session = NULL;
        puts("helper: Stopped prompt session");
    }
    else
    {
        puts("helper: Prompt session stopped by server");
    }
    puts("helper: Done");

    stop_session(&mcd, "helper");
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

    helper(server);
    return 0;
}
