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

#ifndef MIR_CLIENT_LIBRARY_TRUSTED_SESSION_H_
#define MIR_CLIENT_LIBRARY_TRUSTED_SESSION_H_

#include "mir_toolkit/mir_client_library.h"

#include <sys/types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Create an new trusted session
 * \return               Structure that describes the trusted session
 */
MirTrustedSession* mir_trusted_session_create(MirConnection* connection);

/**
 * Create an new trusted session
 * \param [in] session  The trusted session
 * \param [in] pid      The process id of the application to add
 * \return              A MirTrustedSessionAddApplicationResult result of the call
 */
MirTrustedSessionAddApplicationResult mir_trusted_session_add_app_with_pid(MirTrustedSession *session,
    pid_t pid);

/**
 * Register a callback to be called when a Lifecycle state change occurs.
 *   \param [in] session        The trusted session
 *   \param [in] callback       The function to be called when the state change occurs
 *   \param [in,out] context    User data passed to the callback function
 */
void mir_trusted_session_set_event_callback(MirTrustedSession* session,
    mir_trusted_session_event_callback callback, void* context);

/**
 * Request the start of a trusted session
 * \param [in] session  The trusted session
 * \return              True if the request was made successfully,
 *                      false otherwise
 */
MirWaitHandle *mir_trusted_session_start(MirTrustedSession *session, mir_trusted_session_callback callback, void* context);

/**
 * Request the cancellation of a trusted session
 * \param [in] session  The trusted session
 * \return              True if the request was made successfully,
 *                      false otherwise
 */
MirWaitHandle *mir_trusted_session_stop(MirTrustedSession *session, mir_trusted_session_callback callback, void* context);

/**
 * Release a trusted session
 *   \param [in] trusted_session  The trusted session
 */
void mir_trusted_session_release(MirTrustedSession* trusted_session);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_CLIENT_LIBRARY_TRUSTED_SESSION_H_ */
