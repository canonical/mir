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

#ifndef MIR_TOOLKIT_MIR_PRESENTATION_CHAIN_H_
#define MIR_TOOLKIT_MIR_PRESENTATION_CHAIN_H_

#include <mir_toolkit/client_types_nbs.h>

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

/**
 * Create a new presentation chain. 
 *
 * \param [in] connection          A valid connection
 * \param [in] callback            Callback to be invoked when the request
 *                                 completes. The callback is guaranteed to be
 *                                 called and called with a non-null
 *                                 MirPresentationChain*, but the chain may be
 *                                 invalid in case of an error.
 * \param [in] presentation_chain  Userdata to pass to callback function
 * \return                         A handle that can be supplied to mir_wait_for
 */
MirWaitHandle* mir_connection_create_presentation_chain(
    MirConnection* connection,
    mir_presentation_chain_callback callback,
    void* context);

/**
 * Create a new presentation chain and wait for the result.
 *
 * \param [in] connection       A valid connection
 * \return                      The new presentation chain. This is guaranteed
 *                              to be non-null, but may be invalid in the case
 *                              of error.
 */
MirPresentationChain* mir_connection_create_presentation_chain_sync(
    MirConnection* connection);

/**
 * Release the specified presentation chain.
 *   \param [in] presentation_chain    The presentation chain to be released
 */
void mir_presentation_chain_release(MirPresentationChain* presentation_chain);

/** Allocate a MirBuffer and do not wait for the server to return it.
 *
 *  The callback will be called when the buffer is available for use.
 *  It will be called once when created, and once per every
 *  mir_presentation_chain_submit_buffer.
 *
 *   \param [in] presentation_chain    The presentation chain
 *   \param [in] width                 Requested buffer width
 *   \param [in] height                Requested buffer height
 *   \param [in] buffer_usage          Requested buffer usage
 *   \param [in] available_callback    The callback called when the buffer
 *                                     is available
 *   \param [in] available_context     The context for the available_callback
 **/
void mir_presentation_chain_allocate_buffer(
    MirPresentationChain* presentation_chain, 
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    mir_buffer_callback available_callback, void* available_context);

/** Submit a buffer to the server so the server can display it.
 *
 *  The server will notify the client when the buffer is available again via
 *  the callback registered during buffer creation.
 *
 *   \warning: Once submitted, the buffer cannot be modified until the server 
 *             has returned it. There's no guarantee about how long a server
 *             may hold the submitted buffer.
 *
 *   \param [in] presentation_chain  The presentation chain
 *   \param [in] buffer              The buffer to be submitted
 *   \return                         true if the submission succeeded,
 *                                   false if it did not.
 **/
bool mir_presentation_chain_submit_buffer(
    MirPresentationChain* presentation_chain, MirBuffer* buffer);

/**
 * Set the MirSurfaceContent to display a MirPresentationChain.
 *
 * The width and height can be different from the physical dimensions
 * of the MirBuffer's in the MirPresentationChain. If the two differ,
 * the content will be scaled to fit width and height.
 *
 * The initial call to mir_surface_spec_set_presentation_chain or
 * mir_surface_spec_set_buffer_stream will set the bottom-most content,
 * and subsequent calls to either will stack the content on top.
 *
 * \warning         Setting the dimensions or displacements to exceed the 
 *                  bounds of MirSurface may result in clipping to the size
 *                  of the MirSurface, at the server's discretion.
 *
 * \param spec             The surface_spec to be updated.
 * \param width            The width that the content will displayed at.
 * \param height           The height that the content will be displayed at.
 * \param displacement_x   The x displacement from the top-left corner of the MirSurface.
 * \param displacement_y   The y displacement from the top-left corner of the MirSurface.
 * \param chain            The chain containing the content to be displayed.
 */
void mir_surface_spec_set_presentation_chain(
    MirSurfaceSpec* spec,
    int width, int height,
    int displacement_x, int displacement_y,
    MirPresentationChain* chain);

/**
 * Set the MirSurfaceContent to display a MirBufferStream.
 *
 * The initial call to mir_surface_spec_set_presentation_chain or
 * mir_surface_spec_set_buffer_stream will set the bottom-most content,
 * and subsequent calls to either will stack the content on top.
 *
 * \warning         Setting the displacements to exceed the 
 *                  bounds of MirSurface may result in clipping to the size
 *                  of the MirSurface, at the server's discretion.
 *
 * \param spec             The surface_spec to be updated.
 * \param displacement_x   The x displacement from the top-left corner of the MirSurface.
 * \param displacement_y   The y displacement from the top-left corner of the MirSurface.
 * \param stream           The stream containing the content to be displayed.
 */
void mir_surface_spec_set_buffer_stream(
    MirSurfaceSpec* spec,
    int displacement_x, int displacement_y,
    MirBufferStream* stream);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_PRESENTATION_CHAIN_H_
