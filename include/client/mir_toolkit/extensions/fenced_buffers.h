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

#ifndef MIR_CLIENT_EXTENSIONS_FENCED_BUFFERS_H_
#define MIR_CLIENT_EXTENSIONS_FENCED_BUFFERS_H_

#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MirBufferAccess
{
    mir_none,
    mir_read,
    mir_read_write,
} MirBufferAccess;

/** @name Fenced Buffer content access functions.
 *
 * These functions will wait until it is safe to access the buffer for the given purpose.
 * If used with mir_none, the buffer will be given the buffer immediately, and without synchronization.
 * It is then up to the user to ensure that the buffer contents are not accessed at inapproprate times.
 *
 * \note the following functions (mir_buffer_get_native_buffer, mir_buffer_get_graphics_region)
 * can only be used when the buffer is not submitted to the server.
 *  @{ */

/**
 * Retrieve the native fence associated with this buffer
 *
 *   \warning           Take care not to close the fd, the Mir client api
 *                      retains ownership of the fence fd.
 *   \param [in] buffer The buffer.
 *   \return            The fd representing the fence associated with buffer.
 *
 **/
typedef int (*mir_buffer_get_fence)(MirBuffer*);

/**
 * Protect the buffer's contents by associating a native fence with it.
 *
 *   \warning                 any fence currently associated with buffer will be replaced in favor
 *                            of fence without waiting for the replaced fence to clear
 *   \warning                 The Mir client api assumes ownership of the fence fd. 
 *   \param [in] buffer       The buffer
 *   \param [in] fence        The fence that will be associated with buffer. If negative,
 *                            this will remove the fence associated with this buffer.
 *   \param [in] access       The ongoing access that is represented by fence.
 *                            If mir_none is set, this will remove the fence protecting the buffer content.
 **/
typedef void (*mir_buffer_associate_fence)(
    MirBuffer* buffer,
    int fence,
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
typedef int (*mir_buffer_wait_for_access)(
    MirBuffer* buffer,
    MirBufferAccess access,
    int timeout);

typedef struct MirExtensionFencedBuffersV1
{
    mir_buffer_get_fence get_fence;
    mir_buffer_associate_fence associate_fence;
    mir_buffer_wait_for_access wait_for_access;
} MirExtensionFencedBuffersV1;

static inline MirExtensionFencedBuffersV1 const* mir_extension_fenced_buffers_v1(
    MirConnection* connection)
{
    return (MirExtensionFencedBuffersV1 const*) mir_connection_request_extension(
        connection, "mir_extension_fenced_buffers", 1);
}
#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_ANDORID_EGL_H_ */
