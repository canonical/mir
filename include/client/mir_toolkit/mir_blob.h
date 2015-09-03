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

#ifndef MIR_TOOLKIT_MIR_BLOB_H_
#define MIR_TOOLKIT_MIR_BLOB_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Create a blob from a display configuration
 *
 * \param [in] configuration  The display configuration
 * \return                 A blob
 */
MirBlob* mir_blob_from_display_configuration(MirDisplayConfiguration* configuration);

/**
 * Create a blob from a buffer.
 * \note this does not copy the data, the buffer is assumed to be available
 *       until the blob is released.
 *
 * \param [in] buffer      the buffer
 * \param [in] buffer_size the buffer size
 * \return                 A blob
 */
MirBlob* mir_blob_onto_buffer(void const* buffer, size_t buffer_size);

/**
 * Create a blob from a display configuration
 *
 * \warning will abort() if the blob doesn't represent a meaningful display configuration
 *
 * \param [in] blob        The blob
 * \return                 A display configuration
 */
MirDisplayConfiguration* mir_blob_to_display_configuration(MirBlob* blob);

/**
 * Get the size of a blob
 * \param [in] blob        The blob
 * \return                 the size
 */
size_t mir_blob_size(MirBlob* blob);

/**
 * Get the data of a blob
 * \param [in] blob        The blob
 * \return                 the data
 */
void const* mir_blob_data(MirBlob* blob);

/**
 * Release a blob object
 * \param [in] blob        The blob
 */
void mir_blob_release(MirBlob* blob);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_BLOB_H_ */
