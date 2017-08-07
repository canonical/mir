/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_ANDROID_BUFFER_H_
#define MIR_CLIENT_EXTENSIONS_ANDROID_BUFFER_H_

#include "mir_toolkit/mir_connection.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Allocate a MirBuffer usable by the android platform.
 *
 *  The callback will be called when the buffer is available for use.
 *  The buffer can be destroyed via mir_buffer_release().
 *
 *   \note  Not all GRALLOC_USAGE flags or HAL_PIXEL_FORMATs are available.
 *          Be sure to check mir_buffer_is_valid() on the returned buffer.
 *   \param [in] connection            The connection
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] hal_pixel_format      The pixel format, one of
 *                                     Android's HAL_PIXEL_FORMAT*s
 *   \param [in] gralloc_usage_flags   The GRALLOC_USAGE* flags for the buffer.
 *   \param [in] available_callback    The callback called when the buffer
 *                                     is available
 *   \param [in] available_context     The context for the available_callback
 **/
typedef void (*mir_connection_allocate_buffer_android)(
    MirConnection* connection,
    int width, int height,
    unsigned int hal_pixel_format,
    unsigned int gralloc_usage_flags,
    MirBufferCallback available_callback, void* available_context);

typedef struct MirExtensionAndroidBufferV1
{
    mir_connection_allocate_buffer_android allocate_buffer_android;
} MirExtensionAndroidBufferV1;

static inline MirExtensionAndroidBufferV1 const* mir_extension_android_buffer_v1(
    MirConnection* connection)
{
    return (MirExtensionAndroidBufferV1 const*) mir_connection_request_extension(
        connection, "mir_extension_android_buffer", 1);
}

/** Allocate a MirBuffer usable by the android platform and
 *  wait for server response.
 *
 *  The callback will be called when the buffer is available for use.
 *  The buffer can be destroyed via mir_buffer_release().
 *
 *   \note  Not all GRALLOC_USAGE flags or HAL_PIXEL_FORMATs are available.
 *          Be sure to check mir_buffer_is_valid() on the returned buffer.
 *   \param [in] connection            The connection
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] hal_pixel_format      The pixel format, one of
 *                                     Android's HAL_PIXEL_FORMAT*s
 *   \return                           The buffer
 */
typedef MirBuffer* (*MirConnectionAllocateBufferAndroidSync)(
    MirConnection* connection,
    int width, int height,
    unsigned int hal_pixel_format,
    unsigned int gralloc_usage_flags);

/** Check if a MirBuffer is suitable for android usage
 * 
 *   \param [in] buffer The buffer
 *   \return            True if suitable, false if unsuitable
 */
typedef bool (*MirBufferIsAndroidCompatible)(MirBuffer const* buffer);

/** Access the data from the native_handle_t of the MirBuffer
 *   \warning Take care not to close any of the fds.
 *   \pre               The buffer is suitable for android use
 *   \param [in]  buffer    The buffer
 *   \param [out] num_fds   The number of fds
 *   \param [out] fds       The fds
 *   \param [out] num_data  The number of data
 *   \param [out] data      The data
 */
typedef void (*MirBufferAndroidNativeHandle)(
    MirBuffer const* buffer,
    int* num_fds, int const** fds,
    int* num_data, int const** data);

/** Access the HAL_PIXEL_FORMAT of the buffer
 *   \pre                   The buffer is suitable for android use
 *   \param [in]  buffer    The buffer
 *   \return                The hal_pixel_format of the buffer
 */
typedef unsigned int (*MirBufferAndroidHalPixelFormat)(
    MirBuffer const* buffer);

/** Access the GRALLOC_USAGE_FLAGS of the buffer
 *   \pre                   The buffer is suitable for android use
 *   \param [in]  buffer    The buffer
 *   \return                The gralloc_usage of the buffer
 */
typedef unsigned int (*MirBufferAndroidGrallocUsage)(
    MirBuffer const* buffer);

/** Access the stride in bytes of the buffer
 *   \pre                   The buffer is suitable for android use
 *   \param [in]  buffer    The buffer
 *   \return                The stride of the buffer
 */
typedef unsigned int (*MirBufferAndroidStride)(
    MirBuffer const* buffer);

/** Increase refcount of the ANativeWindowBuffer
 *   \pre                   The buffer is suitable for android use
 *   \param [in]  buffer    The buffer
 */
typedef void (*MirBufferAndroidIncRef)(MirBuffer* buffer);

/** Decrease refcount of the ANativeWindowBuffer
 *   \pre                   The buffer is suitable for android use
 *   \param [in]  buffer    The buffer
 */
typedef void (*MirBufferAndroidDecRef)(MirBuffer* buffer);

typedef struct MirExtensionAndroidBufferV2
{
    mir_connection_allocate_buffer_android allocate_buffer_android;
    MirConnectionAllocateBufferAndroidSync allocate_buffer_android_sync;
    MirBufferIsAndroidCompatible is_android_compatible;
    MirBufferAndroidNativeHandle native_handle;
    MirBufferAndroidHalPixelFormat hal_pixel_format;
    MirBufferAndroidGrallocUsage gralloc_usage;
    MirBufferAndroidStride stride;
    MirBufferAndroidIncRef inc_ref;
    MirBufferAndroidDecRef dec_ref;
} MirExtensionAndroidBufferV2;

static inline MirExtensionAndroidBufferV2 const* mir_extension_android_buffer_v2(
    MirConnection* connection)
{
    return (MirExtensionAndroidBufferV2 const*) mir_connection_request_extension(
        connection, "mir_extension_android_buffer", 2);
}
#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_ANDROID_BUFFER_H_ */
