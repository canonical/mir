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

#ifndef MIR_TOOLKIT_MIR_PRESENTATION_CHAIN_H_
#define MIR_TOOLKIT_MIR_PRESENTATION_CHAIN_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Test for a valid presentation chain
 *
 * \param [in] presentation_chain  The presentation chain
 * \return                     True if the supplied presentation_chain is valid,
 *                             or false otherwise.
 */
bool mir_presentation_chain_is_valid(MirPresentationChain* presentation_chain);

/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the chain or the associated
 * connection has been released.
 *   \param [in] presentation_chain    The presentation chain
 *   \return                           A text description of any error
 *                                     resulting in an invalid chain, or the
 *                                     empty string "" if the chain is valid.
 */
char const *mir_presentation_chain_get_error_message(
    MirPresentationChain* presentation_chain);

/** Submit a buffer to the server so the server can display it.
 *
 *  The server will notify the client when the buffer is available again via
 *  the callback registered during buffer creation.
 *
 *   \warning: Once submitted, the buffer cannot be modified or resubmitted
 *             until the server has returned it. There's no guarantee about
 *             how long a server may hold the submitted buffer.
 *
 *   \param [in] presentation_chain     The presentation chain
 *   \param [in] buffer                 The buffer to be submitted
 *   \param [in] available_callback     The callback called when the buffer
 *                                      is available
 *   \param [in] available_context      The context for the available_callback
 **/
void mir_presentation_chain_submit_buffer(
    MirPresentationChain* presentation_chain, MirBuffer* buffer,
    MirBufferCallback available_callback, void* available_context);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_PRESENTATION_CHAIN_H_
