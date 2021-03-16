/*
 * client_types.h: Type definitions used in client apps and libmirclient.
 *
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TOOLKIT_CLIENT_TYPES_H_
#define MIR_TOOLKIT_CLIENT_TYPES_H_
#include <stddef.h>

#ifdef __cplusplus
/**
 * \defgroup mir_toolkit MIR graphics tools API
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



#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_CLIENT_TYPES_H_ */

