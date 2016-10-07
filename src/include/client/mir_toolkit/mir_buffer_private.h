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

#ifndef MIR_TOOLKIT_MIR_BUFFER_PRIVATE_H_
#define MIR_TOOLKIT_MIR_BUFFER_PRIVATE_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <mir_toolkit/client_types_nbs.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/* change the behavior of mir_presentation_chain_submit_buffer
 * to either queue or drop buffers.
 *
 * In the near future, mir_presentation_chain_submit_buffer will just have
 * dropping behavior, but this is needed to accommodate nested passthrough
 */
void mir_presentation_chain_set_queued_submissions(MirPresentationChain* )


/** Suggest parameters to use with EGLCreateImage for a given MirBuffer
 *
 *   \param [in] buffer         The buffer
 *   \param [out] target        The target to use
 *   \param [out] client_buffer The EGLClientBuffer to use 
 *   \param [out] attrs         The attributes to use
 **/
void mir_buffer_egl_image_parameters(
    MirBuffer* buffer,
    EGLenum* target,
    EGLClientBuffer* client_buffer,
    EGLint** attrs);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_BUFFER_PRIVATE_H_
