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
 * Create an new trust session
 * \return               Structure that describes the trust session
 */
MirTrustSession* mir_trust_session_create(MirConnection* connection);

/**
 * Create an new trust session
 * \param [in] session  The trust session
 * \param [in] pid      The process id of the application to add
 * \return              A MirTrustSessionAddApplicationResult result of the call
 */
MirTrustSessionAddApplicationResult mir_trust_session_add_app_with_pid(MirTrustSession *session,
    pid_t pid);

/**
 * Register a callback to be called when a Lifecycle state change occurs.
 *   \param [in] session        The trust session
 *   \param [in] callback       The function to be called when the state change occurs
 *   \param [in,out] context    User data passed to the callback function
 */
void mir_trust_session_set_event_callback(MirTrustSession* session,
    mir_trust_session_event_callback callback, void* context);

/**
 * Request the start of a trust session
 * \param [in] session  The trust session
 * \return              True if the request was made successfully,
 *                      false otherwise
 */
MirWaitHandle *mir_trust_session_start(MirTrustSession *session, mir_trust_session_callback callback, void* context);

/**
 * Request the cancellation of a trust session
 * \param [in] session  The trust session
 * \return              True if the request was made successfully,
 *                      false otherwise
 */
MirWaitHandle *mir_trust_session_stop(MirTrustSession *session, mir_trust_session_callback callback, void* context);

/**
 * Release a trust session
 *   \param [in] trust_session  The trust session
 */
void mir_trust_session_release(MirTrustSession* trust_session);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_CLIENT_LIBRARY_TRUSTED_SESSION_H_ */
