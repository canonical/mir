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

#ifndef MIR_TOOLKIT_MIR_BUFFER_PRIVATE_H_
#define MIR_TOOLKIT_MIR_BUFFER_PRIVATE_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/** Note: mir_presentation_chain{set_queueing,set_dropping}_mode are temporary
 * modes needed by the nested server to support swapinterval on passthrough
 * clients. Once frame notification signals are given to the client, we can
 * just have dropping mode.
 **/

/**
 * Set the MirPresentationChain to display all buffers for at least 1 frame
 * once submitted via mir_presentation_chain_submit_buffer.
 *
 *   \param [in] chain         The chain
 **/
void mir_presentation_chain_set_queueing_mode(MirPresentationChain* chain);

/**
 * Set the MirPresentationChain to return buffers held by the server
 * to the client at the earliest possible time when a new buffer is submitted
 * to mir_presentation_chain_submit_buffer.
 *
 *   \param [in] chain         The chain
 **/
void mir_presentation_chain_set_dropping_mode(MirPresentationChain* chain);

/** Suggest parameters to use with EGLCreateImage for a given MirBuffer
 *
 *   \param [in] buffer         The buffer
 *   \param [out] target        The target to use
 *   \param [out] client_buffer The EGLClientBuffer to use 
 *   \param [out] attrs         The attributes to use
 *   \returns                   True if valid parameters could be found
 **/
bool mir_buffer_get_egl_image_parameters(
    MirBuffer* buffer,
    EGLenum* target,
    EGLClientBuffer* client_buffer,
    EGLint** attrs);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_PRIVATE_H_
