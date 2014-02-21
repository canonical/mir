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

///\page trusted_session_trusted_helper.c trusted_session_trusted_helper.c: A mir client which starts a trusted session and trusted client app.
/// demo_client_trusted_session_trusted_helper shows the use of mir trusted session API.
/// This program opens a mir connection and creates a trusted session.
///\section demo_client_trusted_helper demo_client_trusted_helper()
/// Opens a mir connection and creates a trusted session
/// before closing the trusted session and connection.
/// \example trusted_session_trusted_helper.c A simple mir client
///\section MirDemoState MirDemoState
/// The handles needs to be accessible both to callbacks and to the control function.
/// \snippet trusted_session_trusted_helper.c MirDemoState_tag
///\section demo_client_app2 demo_client_app2()
///\section Callbacks Callbacks
/// This program opens a mir connection and starts a trusted session.
/// \snippet trusted_session_trusted_helper.c Callback_tag

///\internal [MirDemoState_tag]
// Utility structure for the state of a single session.
typedef struct MirDemoState
{
    MirConnection *connection;
    MirTrustedPromptSession *trusted_session;
    MirBool trusted_session_started;
} MirDemoState;
///\internal [MirDemoState_tag]

///\internal [Callback_tag]
// Callback to update MirDemoState on connection
static void connection_callback(MirConnection *new_connection, void *context)
{
    ((MirDemoState*)context)->connection = new_connection;
}

// Callback to update MirDemoState on surface_release
static void trusted_session_started_callback(MirTrustedPromptSession* tps, void* context)
{
    (void)tps;
    ((MirDemoState*)context)->trusted_session_started = mir_true;
}

// Callback to update MirDemoState on surface_release
static void trusted_session_stopped_callback(MirTrustedPromptSession* tps, void* context)
{
    (void)tps;
    ((MirDemoState*)context)->trusted_session_started = mir_false;
}
///\internal [Callback_tag]

void start_session(const char* server, const char* name, MirDemoState* mcd)
{
    ///\internal [connect_tag]
    // Call mir_connect and wait for callback to complete.
    mir_wait_for(mir_connect(server, name, connection_callback, mcd));
    puts("Connected for 'demo_client_trusted_helper'");
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
    if (mcd->trusted_session)
    {
        mir_trusted_prompt_session_release(mcd->trusted_session);
        puts("Trusted session released for 'demo_client_trusted_helper'");
        ///\internal [trusted_session_tag]
    }

    ///\internal [connection_release_tag]
    // We should release our connection
    mir_connection_release(mcd->connection);
    puts("Connection released for 'demo_client_trusted_helper'");
    ///\internal [connection_release_tag]
}

void demo_client_trusted_helper(const char* server, pid_t host_pid, pid_t child_pid)
{
    MirDemoState mcd;
    mcd.connection = 0;
    mcd.trusted_session = 0;
    mcd.trusted_session_started = mir_false;
    start_session(server, "demo_client_trusted_session_trusted_helper", &mcd);

    ///\internal [trusted_session_tag]
    // We create a trusted session
    mcd.trusted_session = mir_trusted_prompt_session_create(mcd.connection);
    assert(mcd.trusted_session != NULL);
    MirTrustedPromptSessionAddApplicationResult add_result = mir_trusted_prompt_session_add_app_with_pid(mcd.trusted_session, host_pid);
    assert(add_result == mir_trusted_prompt_session_app_addition_succeeded);
    add_result = mir_trusted_prompt_session_add_app_with_pid(mcd.trusted_session, child_pid);
    assert(add_result == mir_trusted_prompt_session_app_addition_succeeded);

    mir_wait_for(mir_trusted_prompt_session_start(mcd.trusted_session, trusted_session_started_callback, &mcd));
    assert(mcd.trusted_session_started == mir_true);
    puts("Started Trusted Helper for 'demo_client_trusted_helper'");

    int status;
    printf("Waiting on child app: %d: ", child_pid);
    waitpid(child_pid, &status, 0);

    mir_wait_for(mir_trusted_prompt_session_stop(mcd.trusted_session, trusted_session_stopped_callback, &mcd));
    assert(mcd.trusted_session_started == mir_false);
    puts("Stopped Trusted Helper for 'demo_client_trusted_helper'");

    stop_session(&mcd);
}

// The main() function deals with parsing arguments and defaults
int main(int argc, char* argv[])
{
    // Some variables for holding command line options
    char const *server = NULL;
    pid_t host_pid = 0;

    // Parse the command line
    {
        int arg;
        opterr = 0;
        while ((arg = getopt (argc, argv, "c:hmp:")) != -1)
        {
            switch (arg)
            {
            case 'm':
                server = optarg;
                break;

                case 'p':
                printf("set pid %s\n", optarg);
                host_pid = atoi(optarg);
                break;

            case '?':
            case 'h':
            default:
                puts(argv[0]);
                puts("Usage:");
                puts("    -m <Mir server socket>");
                puts("    -h: this help text");
                puts("    -p: pid of the host app");
                return -1;
            }
        }
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        char *array[] = {"/usr/bin/mir_demo_client_trusted_session_trusted_app2 -s 1280x768"};

        errno = 0;
        int status = execvp("/usr/bin/mir_demo_client_trusted_session_trusted_app2", array);
        printf("binmir_demo_client_trusted_session_trusted_app2: status:%d - errno:%d\n", status, errno);
    }
    else if (pid > 0)
    {
        printf("mir_demo_client_trusted_session_trusted_helper: this:%d , host:%d, child:%d\n", getpid(), host_pid, pid);
        demo_client_trusted_helper(server, host_pid, pid);
    }

    return 0;
}
