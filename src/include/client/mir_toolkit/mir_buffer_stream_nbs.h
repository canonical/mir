/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 */

#ifndef MIR_TOOLKIT_MIR_BUFFER_STREAM_NBS_H_
#define MIR_TOOLKIT_MIR_BUFFER_STREAM_NBS_H_

#include <mir_toolkit/client_types_nbs.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/** Allocate a MirBuffer and do not wait for the server to return it.
 *
 *  The callback will be called when the buffer is available for use.
 *  It will be called once when created, and once per every
 *  mir_buffer_stream_submit_buffer.
 *
 *   \param [in] buffer_stream         The buffer stream
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] buffer_usage          Requested buffer usage
 *   \param [in] available_callback    The callback called when the buffer is available
 *   \param [in] available_context     The context for the available_callback
 **/

void mir_buffer_stream_allocate_buffer(
    MirBufferStream* stream, 
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    mir_buffer_callback available_callback, void* available_context);

/** release a MirBuffer
 *   \param [in] buffer_stream       The buffer stream
 *   \param [in] buffer              The buffer to be released
 **/
void mir_buffer_stream_release_buffer(
    MirBufferStream* stream, MirBuffer* buffer);


/** Submit a buffer to the server so the server can display it.
 *
 *  The server will notify the client when the buffer is available again via the callback
 *  registered during buffer creation.
 *
 *   \warning: Once submitted, the buffer cannot be modified until the server 
 *             has returned it. There's no guarantee about how long a server
 *             may hold the submitted buffer.
 *
 *   \param [in] buffer_stream       The buffer stream
 *   \param [in] buffer              The buffer to be submitted
 *   \return                         true if the submission succeeded,
 *                                   false if it did not.
 **/
bool mir_buffer_stream_submit_buffer(MirBufferStream* buffer_stream, MirBuffer* buffer);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_STREAM_H_
