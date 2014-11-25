/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TOOLKIT_MIR_PROMPT_SESSION_H_
#define MIR_TOOLKIT_MIR_PROMPT_SESSION_H_

#include "mir_toolkit/mir_client_library.h"

#include <sys/types.h>
#include <stdbool.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

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
    mir_prompt_session_state_change_callback state_change_callback,
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
    mir_client_fd_callback callback,
    void * context);

/**
 * Stop and release the specified prompt session
 *   \param [in] prompt_session  The prompt session
 */
void mir_prompt_session_release_sync(MirPromptSession *prompt_session);

/**
 * Test for a valid prompt session
 *   \param [in] prompt_session  The prompt session
 *   \return                     True if prompt_session is valid, false otherwise
 */
bool mir_prompt_session_is_valid(MirPromptSession *prompt_session);

/**
 * Retrieve a text description of the last error. The returned string is owned
 * by the library and remains valid until the prompt session has been released.
 *   \param [in] prompt_session  The prompt session
 *   \return                 A text description of any error resulting in an
 *                           invalid connection, or the empty string "" if the
 *                           connection is valid.
 */
char const *mir_prompt_session_error_message(MirPromptSession *prompt_session);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_PROMPT_SESSION_H_ */
