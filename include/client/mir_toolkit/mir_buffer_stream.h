/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_MIR_BUFFER_STREAM_H_
#define MIR_TOOLKIT_MIR_BUFFER_STREAM_H_

#include <mir_toolkit/mir_native_buffer.h>
#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Test for a valid buffer stream
 *
 * \param [in] buffer_stream  The buffer stream
 * \return                 True if the supplied buffer_stream is valid, or
 *                         false otherwise.
 */
bool mir_buffer_stream_is_valid(MirBufferStream *buffer_stream);

/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the stream or the associated
 * connection has been released.
 *   \param [in] buffer_stream  The buffer stream
 *   \return              A text description of any error resulting in an
 *                        invalid stream, or the empty string "" if the
 *                        connection is valid.
 */
char const *mir_buffer_stream_get_error_message(MirBufferStream *surface);

/**
 * Create a new buffer stream. 
 *
 * For example, the resulting buffer stream may be used
 * with mir_cursor_configuration_from_buffer_stream, 
 * in order to post images to the system cursor.
 *
 * \param [in] connection     A valid connection
 * \param [in] width          Requested buffer width
 * \param [in] height         Requested buffer height
 * \param [in] buffer_usage   Requested buffer usage, use 
 *                            mir_buffer_usage_software for cursor image streams
 * \param [in] callback       Callback to be invoked when the request completes
 *                            The callback is guaranteed to be called and called with a
 *                            non-null MirBufferStream*, but the stream may be invalid in
 *                            case of an error.
 * \param [in] context        Userdata to pass to callback function
 *
 * \return                    A handle that can be supplied to mir_wait_for
 */
MirWaitHandle* mir_connection_create_buffer_stream(MirConnection *connection,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    mir_buffer_stream_callback callback,
    void* context);

/**
 * Create a new buffer stream unattached to a surface and wait for the result. 
 * The resulting buffer stream may be used with 
 * mir_cursor_configuration_from_buffer_stream in order to post images 
 * to the system cursor.
 *
 * \param [in] connection       A valid connection
 * \param [in] width            Requested buffer width
 * \param [in] height           Requested buffer height
 * \param [in] buffer_usage     Requested buffer usage, use 
 *                              mir_buffer_usage_software for cursor image streams
 *
 * \return                      The new buffer stream. This is guaranteed non-null, 
 *                              but may be invalid in the case of error.
 */
MirBufferStream* mir_connection_create_buffer_stream_sync(MirConnection *connection,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage);

/**
 * Release the supplied stream and any associated buffer. The returned wait
 * handle remains valid until the connection to the server is released.
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] stream      The stream
 *   \param [in] callback     Callback function to be invoked when the request
 *                            completes
 *   \param [in,out] context  User data passed to the callback function
 *   \return                  A handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_buffer_stream_release(
    MirBufferStream * buffer_stream,
    mir_buffer_stream_callback callback,
    void *context);

/**
 * Release the specified buffer stream like in mir,_buffer_stream_release(), 
 * but also wait for the operation to complete.
 *   \param [in] buffer stream  The buffer stream to be released
 */
void mir_buffer_stream_release_sync(MirBufferStream *buffer_stream);

/**
 * Get the underlying platform type so the buffer obtained in "raw"
 * representation in mir_buffer_stream_get_current_buffer() 
 * may be understood
 *
 * \deprecated Use of this function is inherently non-portable in the presence
 * of plug-in platform modules as these need not correspond to the available
 * types. To identify the graphics platform use
 * mir_connection_get_graphics_module(). To safely interpret the
 * buffer contents use mir_buffer_stream_get_graphics_region().
 *
 * \todo This should be removed from the public API at the next API break.
 *
 *   \pre                     The surface is valid
 *   \param [in] surface      The surface
 *   \return                  One of mir_platform_type_android or 
 *                            mir_platform_type_gbm
 */
__attribute__ ((deprecated))
MirPlatformType mir_buffer_stream_get_platform_type(MirBufferStream *stream);

/**
 * Retrieve the current buffer in "raw" representation.
 *   \pre                         The buffer stream is valid
 *   \param [in] surface          The buffer stream
 *   \param [out] buffer_package  Structure to be populated
 */
void mir_buffer_stream_get_current_buffer(MirBufferStream *buffer_stream,
    MirNativeBuffer **buffer_package);

/**
 * Advance a buffer stream's buffer. The returned handle remains valid until the
 * next call to mir_buffer_stream_swap_buffers, until the buffer stream has been 
 * released or the connection to the server has been released.
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] buffer_stream      The buffer stream
 *   \param [in] callback     Callback function to be invoked when the request
 *                            completes
 *   \param [in,out] context  User data passed to the callback function
 *   \return                  A handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_buffer_stream_swap_buffers(
    MirBufferStream *buffer_stream,
    mir_buffer_stream_callback callback,
    void *context);

/**
 * Advance a buffer stream's buffer as in mir_buffer stream_swap_buffers(), 
 * but also wait for the operation to complete.
 *   \param [in] buffer stream  The buffer stream whose buffer to advance
 */
void mir_buffer_stream_swap_buffers_sync(MirBufferStream *buffer_stream);

/**
 * Retrieve a buffer stream's graphics region
 *   \warning Depending on platform, this can map the graphics buffer each
 *            time its called. The region remains mapped until
 *            mir_buffer_stream_swap_buffers().
 *   \pre                          The buffer stream is valid
 *   \param [in] buffer stream     The buffer stream
 *   \param [out] graphics_region  Structure to be populated
 */
void mir_buffer_stream_get_graphics_region(
    MirBufferStream *buffer_stream,
    MirGraphicsRegion *graphics_region);

/**
 * Retrieve a window type which may be used by EGL.
 *   \param [in] buffer_stream The buffer stream
 *   \return                   An EGLNativeWindowType that the client can use
 */
MirEGLNativeWindowType mir_buffer_stream_get_egl_native_window(MirBufferStream *buffer_stream);

/**
 * Set the scale associated with all buffers in the stream
 * \param [in] buffer_stream The buffer stream
 * \param [in] scale         The scale
 * \return                  A handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_buffer_stream_set_scale(MirBufferStream* stream, float scale);

/**
 * Set the scale as in mir_buffer_stream_set_scale(), but also wait for the
 * operation to complete.
 * \param [in] buffer_stream The buffer stream
 * \param [in] scale         The scale
 */
void mir_buffer_stream_set_scale_sync(MirBufferStream* stream, float scale);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_STREAM_H_
