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

#ifndef MIR_TOOLKIT_MIR_BUFFER_H_
#define MIR_TOOLKIT_MIR_BUFFER_H_

#include <mir_toolkit/mir_native_buffer.h>
#include <mir_toolkit/client_types_nbs.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/** determine if the content in the MirBuffer is available for the given operation.
 *   \param [in] buffer     The buffer
 *   \return                The fence type protecting this buffer
 **/
MirFenceType mir_buffer_get_fence_type(MirBuffer);

/**
 * Retreive the native fence associated with this buffer
 *
 *   \param [in] buffer     The buffer
 *   \return                The fence associated with buffer 
 *
 **/
MirNativeFence* mir_buffer_get_fence(MirBuffer*);

/**
 * Set the native fence associated with this buffer
 *   \warning                 any fence currently associated with buffer will be replaced in favor
 *                            of native_fence without waiting for the replaced fence to clear
 *   \param [in] buffer       The buffer
 *   \param [in] native_fence The fence that will be associated with buffer. If nullptr, this will
 *                            remove the fence associated with this buffer.
 *   \param [in] fence_type   Either mir_read_fence or mir_write_fence, depending on the operation
 *                            on the buffer content that the fence is protecting. If mir_no_fence is
 *                            set, this will remove the fence protecting this buffer.
 **/
void mir_buffer_set_fence(MirBuffer* buffer, MirNativeFence* native_fence, MirFenceType fence_type);

/** Wait for the fence associated with the buffer to signal.
 *   \param [in] buffer   The buffer
 *   \param [in] type     The type of fence to clear. 
 *   \param [in] timeout  The amount of time to wait for the fence in nanoseconds,
 *                        or -1 for infinite timeout.
 *   \return              zero when fence was cleared successfully, or
 *                        a negative number when the timeout was reached before the fence signals
 **/
int mir_buffer_clear_fence(MirBuffer* buffer, MirFenceType type, int timeout);

/** Fenced Buffer content access functions.
 *
 * Note: the following functions (mir_buffer_get_native_buffer, mir_buffer_lock, mir_buffer_unlock)
 * can only be used when the buffer is not submitted to the server.
 *
 * These functions will implicitly clear the fences designated when called.
 * If used with mir_no_fence, no fences will be cleared, and the user is left to manage fences
 * to ensure that the buffer contents are not accessed at inapproprate times.
 **/

/** The native buffer associated with MirBuffer
 *  This will clear the buffers fence (allowing for write access)
 *
 *   \param [in] buffer    The buffer
 *   \param [in] type      The type of fence to clear before returning.
 *   \return               The platform-defined native buffer associated with buffer
 *
 *   \warning                 The returned native buffer has the same lifetime as the MirBuffer.
 *                            It must not be deleted or past the lifetime of the buffer.
 **/
MirNativeBuffer* mir_buffer_get_native_buffer(MirBuffer*, MirFenceType type);

/** access a CPU-mapped region associated with a given buffer. If the fence
 *  associated with the buffer needs clearing, this function will wait for the
 *  fence to signal.
 *
 *   \param [in] buffer    The buffer
 *   \param [in] type      The type of fence to clear before returning.
 *   \return               The graphics region associated with the buffer.
 *   \warning: If mir_buffer_stream_submit_buffer() is called on a locked buffer,
 *             corruption may occur. Make sure to call mir_buffer_unlock()
 *             before submission.
 **/
MirGraphicsRegion* mir_buffer_lock(MirBuffer *buffer, MirFenceType type);

/** relinquish access to a CPU-mapped region associated with a buffer.
 *   \param [in] region       The region
 **/
void mir_buffer_unlock(MirGraphicsRegion* region);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_H_
