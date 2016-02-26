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

/** Fenced Buffer content access functions.
 *
 * Note: the following functions (mir_buffer_get_native_buffer, mir_buffer_lock, mir_buffer_unlock)
 * can only be used when the buffer is not submitted to the server.
 *
 * These functions will wait until it is safe to access the buffer for the given purpose.
 * If used with mir_none, the buffer will be given the buffer immediately, and without synchronization.
 * It is then up to the user to ensure that the buffer contents are not accessed at inapproprate times.
 **/

/** Access the native buffer associated with MirBuffer for a given purpose.
 *  This will synchronize the buffer for the given purpose.
 *
 *   \param [in] buffer    The buffer
 *   \param [in] access    The access that is needed for the native buffers content.
 *   \return               The platform-defined native buffer associated with buffer
 *   \warning              The returned native buffer has the same lifetime as the MirBuffer.
 *                         It must not be deleted or past the lifetime of the buffer.
 *   \warning              If mir_none is designated as access, this function will not
 *                         wait for the fence. The user must wait for the fence explicitly
 *                         before using the contents of the buffer.
 **/
MirNativeBuffer* mir_buffer_get_native_buffer(MirBuffer*, MirBufferAccess access);

/** Access a CPU-mapped region associated with a given buffer for the given purpose.
 *  This will synchronize the buffer for the given purpose.
 *
 *   \param [in] buffer    The buffer
 *   \param [in] type      The type of fence to clear before returning.
 *   \return     region   The graphics region associated with the buffer.
 *   \warning  The returned region is only valid until the MirBuffer is
 *             submitted to the server. When the buffer is available again,
 *             this function must be called before accessing the region again.
 *   \warning  If mir_none is designated as access, this function will not
 *             wait for the fence. The user must wait for the fence explicitly
 *             before using the contents of the buffer.
 **/
MirGraphicsRegion mir_buffer_get_graphics_region(MirBuffer* buffer, MirBufferAccess access);

/**
 * Retreive the native fence associated with this buffer
 *
 *   \param [in] buffer     The buffer
 *   \return                The fence associated with buffer 
 *
 **/
MirNativeFence* mir_buffer_get_fence(MirBuffer*);

/**
 * Protect the buffer's contents by associating a native fence with it.
 *
 *   \warning                 any fence currently associated with buffer will be replaced in favor
 *                            of native_fence without waiting for the replaced fence to clear
 *   \param [in] buffer       The buffer
 *   \param [in] native_fence The fence that will be associated with buffer. If nullptr, this will
 *                            remove the fence associated with this buffer.
 *   \param [in] access       The ongoing access that is represented by native_fence.
 *                            If mir_none is set, this will remove the fence protecting the buffer content.
 **/
void mir_buffer_associate_fence(
    MirBuffer* buffer,
    MirNativeFence* native_fence,
    MirBufferAccess access);

/** Wait for the fence associated with the buffer to signal. After returning,
 *  it is permissible to access the buffer's content for the designated purpose in access. 
 *
 *   \param [in] buffer   The buffer
 *   \param [in] access   The access to wait for.
 *   \param [in] timeout  The amount of time to wait for the fence in nanoseconds,
 *                        or -1 for infinite timeout.
 *   \return              zero when fence was cleared successfully, or
 *                        a negative number when the timeout was reached before the fence signals
 **/
int mir_buffer_wait_for_access(
    MirBuffer* buffer,
    MirBufferAccess access,
    int timeout);

/** release a MirBuffer
 *   \param [in] buffer              The buffer to be released
 **/
void mir_buffer_release(MirBuffer* buffer);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_H_
