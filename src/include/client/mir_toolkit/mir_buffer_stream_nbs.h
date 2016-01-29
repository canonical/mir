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

/**
 * Test for a valid buffer context
 *
 * \param [in] buffer_context  The buffer context
 * \return                     True if the supplied buffer_context is valid, or
 *                             false otherwise.
 */
bool mir_buffer_context_is_valid(MirBufferContext* buffer_context);

/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the context or the associated
 * connection has been released.
 *   \param [in] buffer_context    The buffer context
 *   \return                       A text description of any error resulting in
 *                                 an invalid context, or the empty string ""
 *                                 if the connection is valid.
 */
char const *mir_buffer_context_get_error_message(MirBufferContext* context);

/**
 * Create a new buffer context. 
 *
 * \param [in] connection      A valid connection
 * \param [in] callback        Callback to be invoked when the request completes
 *                             The callback is guaranteed to be called and called with a
 *                             non-null MirBufferContext*, but the context may be invalid in
 *                             case of an error.
 * \param [in] buffer_context  Userdata to pass to callback function
 * \return                     A handle that can be supplied to mir_wait_for
 */
MirWaitHandle* mir_connection_create_buffer_context(
    MirConnection* connection,
    mir_buffer_context_callback callback,
    void* context);

/**
 * Create a new buffer context unattached to a surface and wait for the result. 
 * The resulting buffer context may be used with 
 * mir_cursor_configuration_from_buffer_context in order to post images 
 * to the system cursor.
 *
 * \param [in] connection       A valid connection
 * \return                      The new buffer context. This is guaranteed non-null, 
 *                              but may be invalid in the case of error.
 */
MirBufferContext* mir_connection_create_buffer_context_sync(MirConnection* connection);

/**
 * Release the specified buffer context.
 *   \param [in] buffer_context  The buffer context to be released
 */
void mir_buffer_context_release(MirBufferContext* buffer_context);

/** Allocate a MirBuffer and do not wait for the server to return it.
 *
 *  The callback will be called when the buffer is available for use.
 *  It will be called once when created, and once per every
 *  mir_buffer_context_submit_buffer.
 *
 *   \param [in] buffer_context        The buffer context
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] buffer_usage          Requested buffer usage
 *   \param [in] available_callback    The callback called when the buffer is available
 *   \param [in] available_context     The context for the available_callback
 **/
void mir_buffer_context_allocate_buffer(
    MirBufferContext* buffer_context, 
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    mir_buffer_callback available_callback, void* available_context);

/** release a MirBuffer
 *   \param [in] buffer_context      The buffer context
 *   \param [in] buffer              The buffer to be released
 **/
void mir_buffer_context_release_buffer(
    MirBufferContext* buffer_context, MirBuffer* buffer);


/** Submit a buffer to the server so the server can display it.
 *
 *  The server will notify the client when the buffer is available again via the callback
 *  registered during buffer creation.
 *
 *   \warning: Once submitted, the buffer cannot be modified until the server 
 *             has returned it. There's no guarantee about how long a server
 *             may hold the submitted buffer.
 *
 *   \param [in] buffer_context      The buffer context
 *   \param [in] buffer              The buffer to be submitted
 *   \return                         true if the submission succeeded,
 *                                   false if it did not.
 **/
bool mir_buffer_context_submit_buffer(MirBufferContext* buffer_context, MirBuffer* buffer);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_STREAM_H_
