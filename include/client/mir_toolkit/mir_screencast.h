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

#ifndef MIR_TOOLKIT_MIR_SCREENCAST_H_
#define MIR_TOOLKIT_MIR_SCREENCAST_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Create a screencast on the supplied connection.
 *
 * A screencast allows clients to read the contents of the screen.
 *
 *   \warning This request may be denied.
 *   \param [in] connection  The connection
 *   \param [in] parameters  The screencast parameters
 *   \return                 The resulting screencast
 */
MirScreencast *mir_connection_create_screencast_sync(
    MirConnection *connection,
    MirScreencastParameters *parameters);

/**
 * Release the specified screencast.
 *   \param [in] screencast  The screencast to be released
 */
void mir_screencast_release_sync(
    MirScreencast *screencast);

/**
 * Retrieve the MirBufferStream associated with a screencast 
 * (to advance buffers, obtain EGLNativeWindowType, etc...)
 * 
 *   \param[in] screencast The screencast
 */
MirBufferStream* mir_screencast_get_buffer_stream(MirScreencast *screencast);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_SCREENCAST_H_ */
