/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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

#ifndef MIR_CLIENT_EXTENSIONS_GBM_BUFFER_H_
#define MIR_CLIENT_EXTENSIONS_GBM_BUFFER_H_

#include "mir_toolkit/mir_connection.h"
#include "mir_toolkit/mir_extension_core.h"
#include "mir_toolkit/mir_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Allocate a MirBuffer via gbm
 *
 *  available in V1 and V2.
 *
 *  The callback will be called when the buffer is available for use.
 *  It will be called once when created, and once per every
 *  mir_presentation_chain_submit_buffer.
 *
 *  The buffer can be destroyed via mir_buffer_release().
 *
 *   \note  Not all formats or flags are available, and allocations may fail.
 *          Be sure to check mir_buffer_is_valid() on the returned buffer.
 *   \param [in] connection            The connection
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] gbm_pixel_format      The pixel format, one of the GBM_FORMATs
 *   \param [in] gbm_bo_flags          The gbm_bo_flags for the buffer.
 *   \param [in] available_callback    The callback called when the buffer
 *                                     is available
 *   \param [in] available_context     The context for the available_callback
 **/
typedef void (*mir_connection_allocate_buffer_gbm)(
    MirConnection* connection,
    int width, int height,
    unsigned int gbm_pixel_format,
    unsigned int gbm_bo_flags,
    MirBufferCallback available_callback, void* available_context);

typedef struct MirExtensionGbmBufferV1
{
    mir_connection_allocate_buffer_gbm allocate_buffer_gbm;
} MirExtensionGbmBufferV1;


/** Allocate a MirBuffer via gbm and wait for the allocation.
 *  available in V2.
 *  The buffer can be destroyed via mir_buffer_release().
 *
 *   \param [in] connection            The connection
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] gbm_pixel_format      The pixel format, one of the GBM_FORMATs
 *   \param [in] gbm_bo_flags          The gbm_bo_flags for the buffer.
 *   \return                           The buffer
 **/
typedef MirBuffer* (*MirConnectionAllocateBufferGbmSync)(
    MirConnection* connection,
    int width, int height,
    unsigned int gbm_pixel_format,
    unsigned int gbm_bo_flags);

/* get the gbm_bo of a given buffer.
 *   \pre   The buffer must be a gbm buffer object.
 *   \param [in] buffer The buffer 
 *   \return            The gbm_bo of the buffer.
 */
typedef struct gbm_bo* (*MirBufferGetGbmBo)(MirBuffer* buffer);

typedef struct MirExtensionGbmBufferV2
{
    mir_connection_allocate_buffer_gbm allocate_buffer_gbm;
    MirConnectionAllocateBufferGbmSync allocate_buffer_gbm_sync;
    MirBufferGetGbmBo get_gbm_bo;
} MirExtensionGbmBufferV2;

static inline MirExtensionGbmBufferV1 const* mir_extension_gbm_buffer_v1(
    MirConnection* connection)
{
    return (MirExtensionGbmBufferV1 const*) mir_connection_request_extension(
        connection, "mir_extension_gbm_buffer", 1);
}

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_GBM_BUFFER_H_ */
