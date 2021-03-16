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

#include "client_types.h"
#include <stdbool.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

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

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_CONNECTION_H_ */

