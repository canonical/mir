/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MIR_TOOLKIT_MIR_CONNECTION_H_
#define MIR_TOOLKIT_MIR_CONNECTION_H_
#include <stdbool.h>
#include <sched.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif
    
/**
 * \addtogroup mir_toolkit
 * @{
 */
/* This is C code. Not C++. */

typedef enum MirPromptSessionState
{
    mir_prompt_session_state_stopped = 0,
    mir_prompt_session_state_started,
    mir_prompt_session_state_suspended
} MirPromptSessionState;


/* Display server connection API */
typedef struct MirConnection MirConnection;
typedef struct MirPromptSession MirPromptSession;


/**
 * Returned by asynchronous functions. Must not be free'd by
 * callers. See the individual function documentation for information
 * on the lifetime of wait handles.
 */
typedef struct MirWaitHandle MirWaitHandle;


/**
 * Callback to be passed when issuing a mir_connect request.
 *   \param [in] connection          the new connection
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*MirConnectedCallback)(
    MirConnection *connection, void *client_context);

/**
 * Callback called when a request for client file descriptors completes
 *   \param [in] prompt_session  The prompt session
 *   \param [in] count          The number of FDs allocated
 *   \param [in] fds            Array of FDs
 *   \param [in,out] context    The context provided by client
 *
 *   \note Copy the FDs as the array will be invalidated after callback completes
 */

typedef void (*MirClientFdCallback)(
    MirPromptSession *prompt_session, size_t count, int const* fds, void* context);

enum { mir_platform_package_max = 32 };

/**
 * Callback member of MirPromptSession for handling of prompt sessions events.
 *   \param [in] prompt_provider  The prompt session associated with the callback
 *   \param [in] state            The state of the prompt session
 *   \param [in,out] context      The context provided by the client
 */
typedef void (*MirPromptSessionStateChangeCallback)(
    MirPromptSession* prompt_provider, MirPromptSessionState state,
    void* context);
    
/**
 * Request a connection to the Mir server. The supplied callback is called when
 * the connection is established, or fails. The returned wait handle remains
 * valid until the connection has been released.
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] server       File path of the server socket to connect to, or
 *                            NULL to choose the default server (can be set by
 *                            the $MIR_SOCKET environment variable)
 *   \param [in] app_name     A name referring to the application
 *   \param [in] callback     Callback function to be invoked when request
 *                            completes
 *   \param [in,out] context  User data passed to the callback function
 *   \return                  A handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_connect(
    char const *server,
    char const *app_name,
    MirConnectedCallback callback,
    void *context);

/**
 * Perform a mir_connect() but also wait for and return the result.
 *   \param [in] server    File path of the server socket to connect to, or
 *                         NULL to choose the default server
 *   \param [in] app_name  A name referring to the application
 *   \return               The resulting MirConnection
 */
MirConnection *mir_connect_sync(char const *server, char const *app_name);

/**
 * Release a connection to the Mir server
 *   \param [in] connection  The connection
 */
void mir_connection_release(MirConnection *connection);


/**
 * Create and start a new prompt session
 *   \param [in] connection             The connection
 *   \param [in] application_pid        The process id of the initiating application
 *   \param [in] state_change_callback  The function to be called when a prompt session state change occurs
 *   \param [in,out] context            User data passed to the callback functions
 *   \return                            A handle that can be passed to mir_wait_for
 */
MirPromptSession *mir_connection_create_prompt_session_sync(
    MirConnection* connection,
    pid_t application_pid,
    MirPromptSessionStateChangeCallback state_change_callback,
    void *context);

/**
 * Allocate some FDs for prompt providers to connect on
 *
 * Prompt helpers need to allocate connection FDs it will pass to
 * prompt providers to use when connecting to the server. The server can
 * then associate them with the prompt session.
 *
 *   \warning This API is tentative until the implementation of prompt sessions is complete
 *   \param [in] prompt_session  The prompt session
 *   \param [in] no_of_fds       The number of fds to allocate
 *   \param [in] callback        Callback invoked when request completes
 *   \param [in,out] context     User data passed to the callback function
 *   \return                     A handle that can be passed to mir_wait_for
 */
MirWaitHandle* mir_prompt_session_new_fds_for_prompt_providers(
    MirPromptSession *prompt_session,
    unsigned int no_of_fds,
    MirClientFdCallback callback,
    void * context);

/**
 * Stop and release the specified prompt session
 *   \param [in] prompt_session  The prompt session
 */
void mir_prompt_session_release_sync(MirPromptSession *prompt_session);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_CONNECTION_H_ */

