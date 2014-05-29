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

#ifndef MIR_TOOLKIT_MIR_TRUST_SESSION_H_
#define MIR_TOOLKIT_MIR_TRUST_SESSION_H_

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
 * Create and start a new trust session
 *   \param [in] connection        The connection
 *   \param [in] base_session_pid  The process id of the initiating application
 *   \param [in] event_callback    The function to be called when a trust session event occurs
 *   \param [in,out] context       User data passed to the callback functions
 *   \return                       A handle that can be passed to mir_wait_for
 */
MirTrustSession *mir_connection_start_trust_session_sync(MirConnection* connection,
    pid_t base_session_pid,
    mir_trust_session_event_callback event_callback,
    void *context);

/**
 * Add a process id to the trust session relationship
 *   \param [in] trust_session  The trust session
 *   \param [in] session_pid    The process id of the application to add
 *   \return                    True if the process id was added, false otherwise
 */
MirBool mir_trust_session_add_trusted_session_sync(MirTrustSession *trust_session,
    pid_t pid);

/**
 * Stop and release the specified trust session
 *   \param [in] trust_session  The trust session
 */
void mir_trust_session_release_sync(MirTrustSession *trust_session);

/**
 * Return the state of trust session
 * \param [in] trust_session  The trust session
 * \return                    The state of the trust session
 */
MirTrustSessionState mir_trust_session_get_state(MirTrustSession *trust_session);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_TRUST_SESSION_H_ */
