/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_MIR_BUFFER_H_
#define MIR_TOOLKIT_MIR_BUFFER_H_

#include <stdbool.h>
#include <mir_toolkit/client_types.h>
#include <mir_toolkit/mir_native_buffer.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/** Allocate a MirBuffer and do not wait for the server to return it.
 *  The buffer will be suitable for writing to via the CPU. Buffers that
 *  will be used on a GPU should be allocated via the platform appropriate
 *  extensions. (eg, mir_extension_gbm_buffer or mir_extension_android_buffer)
 *
 *  The callback will be called when the buffer is created.
 *
 *   \param [in] connection            The connection
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] format                Requested buffer pixel format
 *   \param [in] available_callback    The callback called when the buffer
 *                                     is available
 *   \param [in] available_context     The context for the available_callback
 **/
void mir_connection_allocate_buffer(
    MirConnection* connection,
    int width, int height,
    MirPixelFormat format,
    MirBufferCallback available_callback, void* available_context);

/** Allocate a MirBuffer and wait for the server to return it.
 *
 *   \param [in] connection            The connection
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] format                Requested buffer pixel format
 *   \return                           The buffer
 **/
MirBuffer* mir_connection_allocate_buffer_sync(
    MirConnection* connection,
    int width, int height,
    MirPixelFormat format);

/** Test for a valid buffer
 *   \param [in] buffer    The buffer
 *   \return               True if the buffer is valid, or false otherwise.
 **/
bool mir_buffer_is_valid(MirBuffer const* buffer);

/** Retrieve a text description an error associated with a MirBuffer.
 *  The returned string is owned by the library and remains valid until the
 *  buffer or the associated connection has been released.
 *   \param [in] buffer    The buffer
 *   \return               A text description of any error resulting in an
 *                         invalid buffer, or the empty string "" if the
 *                         connection is valid.
 **/
char const *mir_buffer_get_error_message(MirBuffer const* buffer);

/**
 * Access the MirBufferPackage
 *
 *   \param [in] buffer    The buffer
 *   \return               The MirBufferPackage representing buffer 
 */
MirBufferPackage* mir_buffer_get_buffer_package(MirBuffer* buffer);

/** Access a CPU-mapped region associated with a given buffer.
 *
 *   \param [in] buffer    The buffer
 *   \param [out] region   The mapped region
 *   \param [out] layout   The memory layout of the region
 *   \return               true if success, false if failure
 *   \warning The buffer should be flushed via mir_buffer_munmap() before
 *            submitting the buffer to the server. 
 **/
bool mir_buffer_map(MirBuffer* buffer, MirGraphicsRegion* region, MirBufferLayout* layout);

/** Flush the CPU caches for the buffer.
 *
 *  \post MirGraphicsRegions that are associated with the buffer will be invalid.
 *  \param [in] buffer    The buffer
 **/ 
void mir_buffer_unmap(MirBuffer* buffer);

/** Retrieve the width of the buffer in pixels.
 *
 *   \param [in] buffer   The buffer
 *   \return              The width of the buffer in pixels
 **/
unsigned int mir_buffer_get_width(MirBuffer const* buffer);

/** Retrieve the height of the buffer in pixels.
 *
 *   \param [in] buffer   The buffer
 *   \return              The height of the buffer in pixels
 **/
unsigned int mir_buffer_get_height(MirBuffer const* buffer);

/** Retrieve the pixel format of the buffer.
 *
 *   \param [in] buffer   The buffer
 *   \return              The pixel format of the buffer
 **/
MirPixelFormat mir_buffer_get_pixel_format(MirBuffer const* buffer);

/** @} */

/** release a MirBuffer
 *   \param [in] buffer              The buffer to be released
 **/
void mir_buffer_release(MirBuffer* buffer);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_H_
