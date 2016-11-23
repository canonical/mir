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
 * Authored by:
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
#define MIR_TOOLKIT_MIR_RENDER_SURFACE_H_

#include <mir_toolkit/client_types_nbs.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Create a render surface
 *
 * \param [in] connection                       A valid connection
 * \param [in] width                            The width in pixels
 * \param [in] height                           The height in pixels
 * \param [in] mir_render_surface_callback      Callback to be invoked when the request completes.
 *                                              The callback is guaranteed to be called and called
 *                                              with a non-null MirRenderSurface*, but the render
 *                                              surface may be invalid in case of error.
 * \param [in,out] context                      User data to pass to callback function
 */
void mir_connection_create_render_surface(
    MirConnection* connection,
    int width, int height,
    mir_render_surface_callback callback,
    void* context);

/**
 * Create a render surface and wait for the result
 *
 * \param [in] connection       A valid connection
 * \param [in] width            The width in pixels
 * \param [in] height           The height in pixels
 *
 * \return                      The new render surface, guaranteed to be
 *                              non-null, but may be invalid in case of error
 */
MirRenderSurface* mir_connection_create_render_surface_sync(
    MirConnection* connection,
    int width, int height);

/**
 * Get the size of the MirRenderSurface
 *
 * \param [in] render_surface   The render surface
 * \param [out] width           The width in pixels
 * \param [out] height          The height in pixels
 */ 
void mir_render_surface_get_size(
    MirRenderSurface* render_surface,
    int* width, int* height);

/**
 * Set the size of the MirRenderSurface
 *
 * \param [in] render_surface   The render surface
 * \param [in] width           The width in pixels
 * \param [in] height          The height in pixels
 */ 
void mir_render_surface_set_size(
    MirRenderSurface* render_surface,
    int width, int height);

/**
 * Test for a valid render surface
 *
 * \param [in] render_surface  The render surface
 *
 * \return                     True if the supplied render surface is valid,
 *                             or false otherwise
 */
bool mir_render_surface_is_valid(
    MirRenderSurface* render_surface);

/**
 * Release the specified render surface
 *
 * \param [in] render_surface                   The render surface to be released
 */
void mir_render_surface_release(
    MirRenderSurface* render_surface);

/**
 * Obtain the buffer stream backing a given render surface
 *
 * \param [in] render_surface    The render surface
 * \param [in] width             Requested width
 * \param [in] height            Requested height
 * \param [in] format            Requested pixel format
 * \param [in] usage             Requested buffer usage
 *
 * \return                       The buffer stream contained in the given render surface
 */
MirBufferStream* mir_render_surface_get_buffer_stream(
    MirRenderSurface* render_surface,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage usage);

/**
 * Set the MirSurfaceSpec to display content contained in a render surface
 *
 * \warning: The initial call to mir_surface_spec_add_render_surface will set
 *           the bottom-most content, and subsequent calls will stack the
 *           content on top.
 * \warning: It's an error to call this function with a render surface
 *           that holds no content.
 *
 * \param spec             The surface_spec to be updated
 * \param render_surface   The render surface containing the content to be displayed
 * \param logical_width    The width that the content will be displayed at
 *                         (Ignored for buffer streams)
 * \param logical_height   The height that the content will be displayed at
 *                         (Ignored for buffer streams)
 * \param displacement_x   The x displacement from the top-left corner of the MirSurface
 * \param displacement_y   The y displacement from the top-left corner of the MirSurface
 */
void mir_surface_spec_add_render_surface(
    MirSurfaceSpec* spec,
    MirRenderSurface* render_surface,
    int logical_width, int logical_height,
    int displacement_x, int displacement_y);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
