/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_CLIENT_LIBRARY_LIGHTDM_H
#define MIR_CLIENT_LIBRARY_LIGHTDM_H

#include "mir_toolkit/mir_client_library.h"

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 *  Request a connection to the MIR server. The supplied callback is
 * called when the connection is established, or fails. The returned
 * wait handle remains valid until the connection has been released.
 *  \param [in]     server          a name identifying the server
 *  \param [in]     lightdm_id      an id referring to the application
 *  \param [in]     app_name        a name referring to the application
 *  \param [in]     callback        callback function to be invoked when request completes
 *  \param [in,out] client_context  passed to the callback function
 *  \return a handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_connect_with_lightdm_id(
    char const *server,
    int lightdm_id,
    char const *app_name,
    mir_connected_callback callback,
    void *client_context);


/**
 *  Request focus to be set to a specific application.
 *  \param  [in]   connection   the connection to the mir server
 *  \param  [in]   lightdm_id   an id referring to the application
 *  \return nothing - if an unrecognized id is supplied it is ignored
 */
void mir_select_focus_by_lightdm_id(MirConnection* connection, int lightdm_id);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_CLIENT_LIBRARY_LIGHTDM_H */
